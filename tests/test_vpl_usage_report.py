import csv
import io
import json
import tempfile
import unittest
from contextlib import redirect_stdout
from datetime import datetime
from pathlib import Path
from unittest.mock import patch

from vpl_usage_report import (
    DNSLookupCache,
    analyze_lines,
    main,
    parse_request_log_line,
    parse_time_bound,
    reverse_dns_lookup,
)


class ParseRequestLogLineTests(unittest.TestCase):
    def test_parses_record_without_details(self):
        record = parse_request_log_line(
            "2026-10-09 18:17:58 | 192.0.2.10 | s | available\n"
        )

        self.assertEqual(record.timestamp, datetime(2026, 10, 9, 18, 17, 58))
        self.assertEqual(record.client_ip, "192.0.2.10")
        self.assertTrue(record.secure)
        self.assertEqual(record.request, "available")
        self.assertEqual(record.details, "")

    def test_parses_optional_details(self):
        record = parse_request_log_line(
            "2026-10-09 18:21:25 | 192.0.2.11 | n | running | task-1 RUNNING"
        )

        self.assertFalse(record.secure)
        self.assertEqual(record.details, "task-1 RUNNING")

    def test_rejects_invalid_record(self):
        with self.assertRaisesRegex(ValueError, "TLS field"):
            parse_request_log_line(
                "2026-10-09 18:21:25 | 192.0.2.11 | maybe | running"
            )


class AnalyzeLinesTests(unittest.TestCase):
    def test_summarizes_api_and_maintenance_records(self):
        stats = analyze_lines([
            "2026-10-09 18:17:58 | 192.0.2.10 | s | request | task-1\n",
            "2026-10-09 18:18:00 | 192.0.2.10 | n | running | task-1 RUNNING\n",
            "2026-10-09 18:19:00 | local | n | Clean task | task-1\n",
            "not a request log line\n",
        ])

        self.assertEqual(stats.records, 3)
        self.assertEqual(stats.api_requests, 2)
        self.assertEqual(stats.task_starts, 1)
        self.assertEqual(stats.maintenance_events, 1)
        self.assertEqual(stats.tls_requests, 1)
        self.assertEqual(stats.plain_requests, 1)
        self.assertEqual(stats.client_counts["192.0.2.10"], 2)
        self.assertEqual(stats.malformed_lines, 1)

    def test_date_bounds_include_the_entire_until_day(self):
        stats = analyze_lines(
            [
                "2026-10-09 00:00:00 | 192.0.2.10 | s | request\n",
                "2026-10-10 23:59:59 | 192.0.2.10 | s | request\n",
                "2026-10-11 00:00:00 | 192.0.2.10 | s | request\n",
            ],
            since=parse_time_bound("2026-10-09"),
            until=parse_time_bound("2026-10-10"),
        )

        self.assertEqual(stats.api_requests, 2)

    def test_csv_export_contains_normalized_records(self):
        output = io.StringIO()
        writer = csv.writer(output)
        stats = analyze_lines(
            ["2026-10-09 18:17:58 | local | n | Clean task | task-1\n"],
            csv_writer=writer,
        )

        self.assertEqual(stats.maintenance_events, 1)
        self.assertEqual(
            output.getvalue().strip(),
            "2026-10-09 18:17:58,local,,no,Clean task,task-1,maintenance",
        )

    def test_reverse_dns_is_cached_and_written_to_csv(self):
        output = io.StringIO()
        writer = csv.writer(output)
        lines = [
            "2026-10-09 18:17:58 | 192.0.2.10 | s | request | task-1\n",
            "2026-10-09 18:18:00 | 192.0.2.10 | n | running | task-1 RUNNING\n",
        ]
        cache = DNSLookupCache(Path(tempfile.gettempdir()) / "unused-dns-cache")

        with patch(
            "vpl_usage_report.socket.gethostbyaddr",
            return_value=("student.example.edu.", [], ["192.0.2.10"]),
        ) as gethostbyaddr:
            stats = analyze_lines(
                lines,
                csv_writer=writer,
                resolve_hostnames=True,
                dns_cache=cache,
            )

        self.assertEqual(stats.hostnames, {"192.0.2.10": "student.example.edu"})
        self.assertEqual(cache.entries["192.0.2.10"][0], "student.example.edu")
        self.assertEqual(gethostbyaddr.call_count, 1)
        self.assertIn(
            "2026-10-09 18:17:58,192.0.2.10,student.example.edu,yes,request,task-1,request",
            output.getvalue(),
        )
    def test_reverse_dns_failure_falls_back_to_ip(self):
        with patch(
            "vpl_usage_report.socket.gethostbyaddr",
            side_effect=OSError("no PTR record"),
        ):
            stats = analyze_lines(
                ["2026-10-09 18:17:58 | 192.0.2.10 | s | request\n"],
                resolve_hostnames=True,
            )

        self.assertIsNone(stats.hostnames["192.0.2.10"])

    def test_reverse_dns_rejects_non_ip_client_values_without_lookup(self):
        with patch("vpl_usage_report.socket.gethostbyaddr") as gethostbyaddr:
            self.assertIsNone(reverse_dns_lookup("not-an-ip"))
        gethostbyaddr.assert_not_called()

    def test_dns_cache_persists_unexpired_lookups(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            cache_path = Path(temporary_directory) / "dns_cache"
            cache = DNSLookupCache(cache_path)
            with patch(
                "vpl_usage_report.socket.gethostbyaddr",
                return_value=("student.example.edu.", [], ["192.0.2.10"]),
            ) as gethostbyaddr:
                self.assertEqual(
                    cache.lookup("192.0.2.10"), "student.example.edu"
                )
            cache.save()

            loaded_cache = DNSLookupCache(cache_path)
            loaded_cache.load()
            with patch("vpl_usage_report.socket.gethostbyaddr") as gethostbyaddr:
                self.assertEqual(
                    loaded_cache.lookup("192.0.2.10"), "student.example.edu"
                )
            gethostbyaddr.assert_not_called()
            self.assertEqual(
                cache_path.parent.stat().st_mode & 0o777,
                0o700,
            )
            self.assertEqual(cache_path.stat().st_mode & 0o777, 0o600)
            self.assertEqual(gethostbyaddr.call_count, 0)

    def test_expired_dns_cache_entry_is_refreshed(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            cache_path = Path(temporary_directory) / "dns_cache"
            cache_path.write_text(
                json.dumps({
                    "192.0.2.10": {
                        "hostname": "old.example.edu",
                        "expires_at": 0,
                    }
                }),
                encoding="utf-8",
            )
            cache = DNSLookupCache(cache_path)
            cache.load()
            with patch(
                "vpl_usage_report.socket.gethostbyaddr",
                return_value=("new.example.edu.", [], ["192.0.2.10"]),
            ) as gethostbyaddr:
                self.assertEqual(cache.lookup("192.0.2.10"), "new.example.edu")

        gethostbyaddr.assert_called_once_with("192.0.2.10")
        self.assertTrue(cache.dirty)


class CommandLineTests(unittest.TestCase):
    def test_cli_resolves_and_displays_hostnames_by_default(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            log_file = Path(temporary_directory) / "requests.log"
            log_file.write_text(
                "2026-10-09 18:17:58 | 192.0.2.10 | s | request | task-1\n",
                encoding="utf-8",
            )
            stdout = io.StringIO()

            with patch(
                "vpl_usage_report.DEFAULT_DNS_CACHE_PATH",
                Path(temporary_directory) / "dns_cache",
            ):
                with patch(
                    "vpl_usage_report.socket.gethostbyaddr",
                    return_value=("student.example.edu.", [], ["192.0.2.10"]),
                ):
                    with redirect_stdout(stdout):
                        result = main([str(log_file)])

            self.assertEqual(result, 0)
            self.assertIn(
                "192.0.2.10 (student.example.edu): 1", stdout.getvalue()
            )
            self.assertIn("Reverse DNS: enabled", stdout.getvalue())

    def test_cli_prints_report_and_writes_filtered_csv(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            log_file = directory / "requests.log"
            csv_file = directory / "report.csv"
            log_file.write_text(
                "2026-10-09 18:17:58 | 192.0.2.10 | s | request | task-1\n"
                "2026-10-10 18:17:58 | 192.0.2.11 | n | available\n",
                encoding="utf-8",
            )
            stdout = io.StringIO()

            with redirect_stdout(stdout):
                result = main([
                    str(log_file),
                    "--since",
                    "2026-10-09",
                    "--until",
                    "2026-10-09",
                    "--csv-out",
                    str(csv_file),
                    "--no-reverse-dns",
                ])

            self.assertEqual(result, 0)
            self.assertIn("API requests: 1", stdout.getvalue())
            self.assertIn("Task starts (request + directrun): 1", stdout.getvalue())
            self.assertIn("CSV records written to", stdout.getvalue())
            with csv_file.open(encoding="utf-8", newline="") as source:
                self.assertEqual(len(list(csv.reader(source))), 2)


if __name__ == "__main__":
    unittest.main()
