#!/usr/bin/env python3
"""Summarize VPL Jail System request-log activity."""

import argparse
import csv
import ipaddress
import json
import os
import socket
import sys
import tempfile
import time as time_module
from collections import Counter
from dataclasses import dataclass, field
from datetime import date, datetime, time, timedelta
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, TextIO, Tuple


DEFAULT_LOG_PATH = Path("/var/log/vpl/vpl-jail-server.log")
DEFAULT_DNS_CACHE_PATH = Path.home() / ".vpl_usage_report" / "dns_cache"
DNS_CACHE_TTL_SECONDS = 7 * 24 * 60 * 60
CSV_FIELDS = (
    "timestamp", "client_ip", "hostname", "tls", "request", "details", "category"
)


@dataclass(frozen=True)
class TimeBound:
    value: datetime
    is_date: bool
    source: str


@dataclass(frozen=True)
class RequestLogRecord:
    timestamp: datetime
    client_ip: str
    secure: bool
    request: str
    details: str

    @property
    def is_maintenance(self) -> bool:
        return self.client_ip == "local"


@dataclass
class UsageStats:
    records: int = 0
    api_requests: int = 0
    maintenance_events: int = 0
    malformed_lines: int = 0
    request_counts: Counter = field(default_factory=Counter)
    daily_counts: Counter = field(default_factory=Counter)
    client_counts: Counter = field(default_factory=Counter)
    tls_requests: int = 0
    plain_requests: int = 0
    clients: Set[str] = field(default_factory=set)
    hostnames: Dict[str, Optional[str]] = field(default_factory=dict)
    first_event: Optional[datetime] = None
    last_event: Optional[datetime] = None
    malformed_examples: List[Tuple[int, str]] = field(default_factory=list)

    @property
    def task_starts(self) -> int:
        return self.request_counts["request"] + self.request_counts["directrun"]


def parse_time_bound(value: str) -> TimeBound:
    """Parse a local date or timezone-naive ISO datetime from the log's clock."""
    if len(value) == 10:
        try:
            parsed_date = date.fromisoformat(value)
        except ValueError as error:
            raise argparse.ArgumentTypeError(
                "expected YYYY-MM-DD or a local ISO datetime"
            ) from error
        return TimeBound(datetime.combine(parsed_date, time.min), True, value)

    try:
        parsed_datetime = datetime.fromisoformat(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError(
            "expected YYYY-MM-DD or a local ISO datetime"
        ) from error
    if parsed_datetime.utcoffset() is not None:
        raise argparse.ArgumentTypeError(
            "use a timezone-naive local datetime; the log has no timezone offset"
        )
    return TimeBound(parsed_datetime, False, value)


def parse_request_log_line(line: str) -> RequestLogRecord:
    """Parse one record written by Logger::logRequest."""
    fields = line.rstrip("\r\n").split(" | ", 4)
    if len(fields) not in (4, 5):
        raise ValueError("expected four required fields and an optional details field")

    try:
        timestamp = datetime.strptime(fields[0], "%Y-%m-%d %H:%M:%S")
    except ValueError as error:
        raise ValueError("invalid timestamp") from error

    client_ip, secure_field, request = fields[1:4]
    if not client_ip:
        raise ValueError("missing client address")
    if secure_field not in ("s", "n"):
        raise ValueError("TLS field must be 's' or 'n'")
    if not request:
        raise ValueError("missing request name")

    details = fields[4] if len(fields) == 5 else ""
    return RequestLogRecord(
        timestamp=timestamp,
        client_ip=client_ip,
        secure=secure_field == "s",
        request=request,
        details=details,
    )


def is_in_range(
    timestamp: datetime,
    since: Optional[TimeBound],
    until: Optional[TimeBound],
) -> bool:
    if since is not None and timestamp < since.value:
        return False
    if until is not None:
        if until.is_date:
            if timestamp.date() > until.value.date():
                return False
        elif timestamp > until.value:
            return False
    return True


def reverse_dns_lookup(client_ip: str) -> Optional[str]:
    """Return a client's PTR hostname, or None when no usable name is available."""
    try:
        ipaddress.ip_address(client_ip)
    except ValueError:
        return None

    try:
        hostname = socket.gethostbyaddr(client_ip)[0].rstrip(".")
    except OSError:
        return None
    return hostname or None


class DNSLookupCache:
    """Persistent reverse-DNS cache with a fixed time-to-live."""

    def __init__(
        self,
        path: Path = DEFAULT_DNS_CACHE_PATH,
        ttl_seconds: int = DNS_CACHE_TTL_SECONDS,
    ) -> None:
        self.path = path
        self.ttl_seconds = ttl_seconds
        self.entries: Dict[str, Tuple[Optional[str], float]] = {}
        self.warnings: List[str] = []
        self.dirty = False

    def load(self) -> None:
        if self.path.is_symlink():
            self.warnings.append(
                "DNS cache path is a symbolic link; ignoring {}".format(self.path)
            )
            return
        try:
            with self.path.open("r", encoding="utf-8") as cache_file:
                data = json.load(cache_file)
        except FileNotFoundError:
            return
        except (OSError, json.JSONDecodeError) as error:
            self.warnings.append(
                "Cannot read DNS cache {}: {}".format(self.path, error)
            )
            return

        if not isinstance(data, dict):
            self.warnings.append(
                "DNS cache {} has an invalid format; ignoring it".format(self.path)
            )
            return

        now = time_module.time()
        invalid_entries = 0
        for client_ip, value in data.items():
            if not isinstance(client_ip, str) or not isinstance(value, dict):
                invalid_entries += 1
                continue
            hostname = value.get("hostname")
            expires_at = value.get("expires_at")
            if (
                (hostname is not None and not isinstance(hostname, str))
                or isinstance(expires_at, bool)
                or not isinstance(expires_at, (int, float))
            ):
                invalid_entries += 1
                continue
            try:
                ipaddress.ip_address(client_ip)
            except ValueError:
                invalid_entries += 1
                continue
            if expires_at > now:
                self.entries[client_ip] = (hostname, float(expires_at))
            else:
                self.dirty = True
        if invalid_entries:
            self.warnings.append(
                "Ignored {} invalid entr{} in DNS cache {}".format(
                    invalid_entries,
                    "y" if invalid_entries == 1 else "ies",
                    self.path,
                )
            )

    def lookup(self, client_ip: str) -> Optional[str]:
        try:
            ipaddress.ip_address(client_ip)
        except ValueError:
            return None

        now = time_module.time()
        entry = self.entries.get(client_ip)
        if entry is not None and entry[1] > now:
            return entry[0]
        if entry is not None:
            del self.entries[client_ip]

        hostname = reverse_dns_lookup(client_ip)
        self.entries[client_ip] = (hostname, now + self.ttl_seconds)
        self.dirty = True
        return hostname

    def save(self) -> None:
        if not self.dirty:
            return

        parent = self.path.parent
        if parent.is_symlink():
            raise OSError("DNS cache directory must not be a symbolic link")
        parent.mkdir(mode=0o700, parents=True, exist_ok=True)
        if not parent.is_dir():
            raise OSError("DNS cache parent is not a directory: {}".format(parent))
        os.chmod(parent, 0o700)

        temporary_path: Optional[Path] = None
        try:
            with tempfile.NamedTemporaryFile(
                mode="w",
                encoding="utf-8",
                dir=str(parent),
                prefix=".dns_cache.",
                delete=False,
            ) as temporary_file:
                temporary_path = Path(temporary_file.name)
                now = time_module.time()
                data = {
                    client_ip: {
                        "hostname": hostname,
                        "expires_at": expires_at,
                    }
                    for client_ip, (hostname, expires_at) in self.entries.items()
                    if expires_at > now
                }
                json.dump(data, temporary_file, sort_keys=True)
                temporary_file.write("\n")
                temporary_file.flush()
                os.fsync(temporary_file.fileno())
            os.replace(str(temporary_path), str(self.path))
            self.dirty = False
        finally:
            if temporary_path is not None and temporary_path.exists():
                temporary_path.unlink()


def write_csv_record(
    writer: csv.writer,
    record: RequestLogRecord,
    hostname: Optional[str] = None,
) -> None:
    writer.writerow((
        record.timestamp.isoformat(sep=" "),
        record.client_ip,
        hostname or "",
        "yes" if record.secure else "no",
        record.request,
        record.details,
        "maintenance" if record.is_maintenance else "request",
    ))


def analyze_lines(
    lines: Iterable[str],
    since: Optional[TimeBound] = None,
    until: Optional[TimeBound] = None,
    csv_writer: Optional[csv.writer] = None,
    resolve_hostnames: bool = False,
    dns_cache: Optional[DNSLookupCache] = None,
) -> UsageStats:
    stats = UsageStats()

    for line_number, line in enumerate(lines, start=1):
        if not line.strip():
            continue
        try:
            record = parse_request_log_line(line)
        except ValueError as error:
            stats.malformed_lines += 1
            if len(stats.malformed_examples) < 5:
                stats.malformed_examples.append((line_number, str(error)))
            continue

        if not is_in_range(record.timestamp, since, until):
            continue

        stats.records += 1
        if stats.first_event is None or record.timestamp < stats.first_event:
            stats.first_event = record.timestamp
        if stats.last_event is None or record.timestamp > stats.last_event:
            stats.last_event = record.timestamp
        hostname = ""
        if not record.is_maintenance and resolve_hostnames:
            if record.client_ip not in stats.hostnames:
                stats.hostnames[record.client_ip] = (
                    dns_cache.lookup(record.client_ip)
                    if dns_cache is not None
                    else reverse_dns_lookup(record.client_ip)
                )
            hostname = stats.hostnames[record.client_ip] or ""
        if csv_writer is not None:
            write_csv_record(writer=csv_writer, record=record, hostname=hostname)

        if record.is_maintenance:
            stats.maintenance_events += 1
            continue

        stats.api_requests += 1
        stats.request_counts[record.request] += 1
        stats.daily_counts[record.timestamp.date()] += 1
        stats.client_counts[record.client_ip] += 1
        stats.clients.add(record.client_ip)
        if record.secure:
            stats.tls_requests += 1
        else:
            stats.plain_requests += 1

    return stats


def validate_time_range(
    parser: argparse.ArgumentParser,
    since: Optional[TimeBound],
    until: Optional[TimeBound],
) -> None:
    if since is None or until is None:
        return
    if until.is_date:
        if since.value.date() > until.value.date():
            parser.error("--since must be on or before --until")
    elif since.value > until.value:
        parser.error("--since must be on or before --until")


def render_report(
    stats: UsageStats,
    log_path: Path,
    since: Optional[TimeBound],
    until: Optional[TimeBound],
    top_clients: int,
    resolve_hostnames: bool,
) -> str:
    lines = [
        "VPL Jail System usage report",
        "Log: {}".format(log_path),
    ]
    if since is not None or until is not None:
        lines.append(
            "Range: {} through {}".format(
                since.source if since is not None else "beginning",
                until.source if until is not None else "latest",
            )
        )
    if stats.first_event is not None and stats.last_event is not None:
        lines.append(
            "Events covered: {} through {}".format(
                stats.first_event.isoformat(sep=" "),
                stats.last_event.isoformat(sep=" "),
            )
        )

    lines.extend((
        "",
        "Logged records: {}".format(stats.records),
        "API requests: {}".format(stats.api_requests),
        "Task starts (request + directrun): {}".format(stats.task_starts),
        "Distinct client IPs: {}".format(len(stats.clients)),
        "TLS requests: {}".format(stats.tls_requests),
        "Non-TLS requests: {}".format(stats.plain_requests),
        "Maintenance events: {}".format(stats.maintenance_events),
        "Reverse DNS: {}".format("enabled" if resolve_hostnames else "disabled"),
    ))

    lines.append("")
    lines.append("Requests by method:")
    if stats.request_counts:
        for request, count in sorted(
            stats.request_counts.items(), key=lambda item: (-item[1], item[0])
        ):
            lines.append("  {}: {}".format(request, count))
    else:
        lines.append("  (none)")

    lines.append("")
    lines.append("Requests by day:")
    if stats.daily_counts:
        for day in sorted(stats.daily_counts):
            lines.append("  {}: {}".format(day.isoformat(), stats.daily_counts[day]))
    else:
        lines.append("  (none)")

    lines.append("")
    lines.append("Top clients:")
    top = sorted(
        stats.client_counts.items(), key=lambda item: (-item[1], item[0])
    )[:top_clients]
    if top:
        for client_ip, count in top:
            hostname = stats.hostnames.get(client_ip)
            label = "{} ({})".format(client_ip, hostname) if hostname else client_ip
            lines.append("  {}: {}".format(label, count))
    else:
        lines.append("  (none)")

    if stats.malformed_lines:
        lines.append("")
        lines.append("Malformed lines ignored: {}".format(stats.malformed_lines))
        for line_number, reason in stats.malformed_examples:
            lines.append("  Line {}: {}".format(line_number, reason))
        remaining = stats.malformed_lines - len(stats.malformed_examples)
        if remaining:
            lines.append("  ... and {} more".format(remaining))

    return "\n".join(lines)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Report usage recorded in the VPL Jail System request log."
    )
    parser.add_argument(
        "log_file",
        nargs="?",
        type=Path,
        default=DEFAULT_LOG_PATH,
        help="request log path (default: {})".format(DEFAULT_LOG_PATH),
    )
    parser.add_argument(
        "--since",
        type=parse_time_bound,
        metavar="DATE_OR_DATETIME",
        help="include events from this local date/time (inclusive)",
    )
    parser.add_argument(
        "--until",
        type=parse_time_bound,
        metavar="DATE_OR_DATETIME",
        help="include events through this local date/time (inclusive)",
    )
    parser.add_argument(
        "--csv-out",
        type=Path,
        metavar="PATH",
        help="write filtered normalized log records to a CSV file",
    )
    parser.add_argument(
        "--top-clients",
        type=int,
        default=10,
        metavar="N",
        help="number of client IPs to show (default: 10)",
    )
    parser.add_argument(
        "--no-reverse-dns",
        dest="resolve_hostnames",
        action="store_false",
        help="skip PTR hostname lookups (reverse DNS is enabled by default)",
    )
    parser.set_defaults(resolve_hostnames=True)
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.top_clients < 1:
        parser.error("--top-clients must be a positive integer")
    validate_time_range(parser, args.since, args.until)

    if args.csv_out is not None:
        try:
            same_file = args.log_file.samefile(args.csv_out)
        except FileNotFoundError:
            same_file = False
        if same_file or args.log_file.absolute() == args.csv_out.absolute():
            parser.error("CSV output must be different from the input log")

    dns_cache = (
        DNSLookupCache(DEFAULT_DNS_CACHE_PATH) if args.resolve_hostnames else None
    )
    if dns_cache is not None:
        dns_cache.load()

    try:
        with args.log_file.open("r", encoding="utf-8") as log_file:
            if args.csv_out is None:
                stats = analyze_lines(
                    log_file,
                    args.since,
                    args.until,
                    resolve_hostnames=args.resolve_hostnames,
                    dns_cache=dns_cache,
                )
            else:
                with args.csv_out.open("w", encoding="utf-8", newline="") as csv_file:
                    writer = csv.writer(csv_file)
                    writer.writerow(CSV_FIELDS)
                    stats = analyze_lines(
                        log_file,
                        args.since,
                        args.until,
                        csv_writer=writer,
                        resolve_hostnames=args.resolve_hostnames,
                        dns_cache=dns_cache,
                    )
    except OSError as error:
        parser.error("cannot read/write report files: {}".format(error))
    except UnicodeError as error:
        parser.error("request log is not valid UTF-8: {}".format(error))

    if dns_cache is not None:
        try:
            dns_cache.save()
        except OSError as error:
            dns_cache.warnings.append(
                "Cannot save DNS cache {}: {}".format(dns_cache.path, error)
            )
        for warning in dns_cache.warnings:
            print("Warning: {}".format(warning), file=sys.stderr)

    print(render_report(
        stats,
        args.log_file,
        args.since,
        args.until,
        args.top_clients,
        args.resolve_hostnames,
    ))
    if args.csv_out is not None:
        print("CSV records written to {}".format(args.csv_out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
