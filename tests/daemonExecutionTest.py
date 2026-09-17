#!/usr/bin/env python3
"""Compile and run a C++ program through a running VPL jail daemon."""

import argparse
import asyncio
import importlib
import json
import sys
import time
from urllib.error import HTTPError, URLError
from urllib.parse import urlsplit, urlunsplit
from urllib.request import Request, urlopen


EXPECTED_OUTPUT = "daemon-integration-ok\n"
INTERACTIVE_INPUT = "daemon-interactive-input"
INTERACTIVE_OUTPUT = "received: " + INTERACTIVE_INPUT
EXPECTED_INTERACTIVE_OUTPUT = "input: " + INTERACTIVE_INPUT + "\n" + INTERACTIVE_OUTPUT + "\n"

try:
    websockets = importlib.import_module("websockets")
except ImportError:
    websockets = None


def call(url, method, params, request_id):
    payload = json.dumps({
        "jsonrpc": "2.0",
        "method": method,
        "params": params,
        "id": request_id,
    }).encode("utf-8")
    request = Request(url, data=payload, headers={"Content-Type": "application/json"})
    try:
        with urlopen(request, timeout=10) as response:
            reply = json.load(response)
    except (HTTPError, URLError, OSError, json.JSONDecodeError) as error:
        raise RuntimeError("%s request failed: %s" % (method, error))
    if "error" in reply:
        raise RuntimeError("%s request returned an error: %s" % (method, reply["error"]))
    return reply["result"]


def request_params(interactive=False):
    program = "#include <iostream>\nint main() { std::cout << \"daemon-integration-ok\\n\"; }\n"
    if interactive:
        program = (
            "#include <iostream>\n"
            "#include <string>\n"
            "int main() {\n"
            "    std::string input;\n"
            "    std::cout << \"input: \" << std::flush;\n"
            "    std::getline(std::cin, input);\n"
            "    std::cout << \"received: \" << input << std::endl;\n"
            "}\n")
    return {
        "maxtime": 15,
        "maxfilesize": 1048576,
        "maxmemory": 134217728,
        "maxprocesses": 10,
        "runscript": "",
        "debugscript": "",
        "userid": "daemon-test",
        "activityid": "daemon-test",
        "execute": "compile.sh",
        "interactive": int(interactive),
        "lang": "C.UTF-8",
        "pluginversion": 2021052513,
        "files": {
            "compile.sh": "#!/bin/sh\nexec g++ -std=c++11 -o vpl_execution hello.cpp\n",
            "hello.cpp": program,
        },
        "filestodelete": {"compile.sh": 1},
        "fileencoding": {"compile.sh": 0, "hello.cpp": 0},
    }


def monitor_url(url, monitorticket):
    parts = urlsplit(url)
    scheme = "wss" if parts.scheme == "https" else "ws"
    return urlunsplit((scheme, parts.netloc, "/%s/monitor" % monitorticket, "", ""))


def execute_url(url, executionticket):
    parts = urlsplit(url)
    scheme = "wss" if parts.scheme == "https" else "ws"
    return urlunsplit((scheme, parts.netloc, "/%s/execute" % executionticket, "", ""))


async def monitor_execution(url, monitorticket, timeout):
    if websockets is None:
        raise RuntimeError("the Python websockets package is required for daemon monitoring")
    messages = []
    deadline = time.monotonic() + timeout
    try:
        async with websockets.connect(monitor_url(url, monitorticket), open_timeout=10) as websocket:
            while time.monotonic() < deadline:
                try:
                    message = await asyncio.wait_for(websocket.recv(), deadline - time.monotonic())
                except asyncio.TimeoutError:
                    break
                except websockets.ConnectionClosed:
                    break
                messages.append(message)
                print("Monitor: %s" % message)
                if message == "retrieve:":
                    break
    except OSError as error:
        raise RuntimeError("monitor WebSocket connection failed: %s" % error)
    print("Monitor finished with %d messages" % len(messages))
    return messages


async def run_interactive_test(url, monitorticket, executionticket, timeout):
    if websockets is None:
        raise RuntimeError("the Python websockets package is required for interactive execution")
    deadline = time.monotonic() + timeout
    try:
        async with websockets.connect(monitor_url(url, monitorticket), open_timeout=10) as monitor:
            while time.monotonic() < deadline:
                message = await asyncio.wait_for(monitor.recv(), deadline - time.monotonic())
                print("Monitor: %s" % message)
                if message == "run:terminal":
                    break
            else:
                raise RuntimeError("monitor did not start an interactive terminal")
            async with websockets.connect(execute_url(url, executionticket), open_timeout=10) as terminal:
                output = ""
                while "input: " not in output and time.monotonic() < deadline:
                    output += await asyncio.wait_for(terminal.recv(), deadline - time.monotonic())
                if "input: " not in output:
                    raise RuntimeError("interactive terminal did not request input: %r" % output)
                await terminal.send(INTERACTIVE_INPUT + "\n")
                while time.monotonic() < deadline:
                    try:
                        output += await asyncio.wait_for(terminal.recv(), deadline - time.monotonic())
                    except websockets.ConnectionClosed:
                        break
                output = output.replace("\r\n", "\n").replace("\r", "\n")
                if output != EXPECTED_INTERACTIVE_OUTPUT:
                    raise RuntimeError("unexpected interactive terminal output: %r" % output)
    except (OSError, asyncio.TimeoutError) as error:
        raise RuntimeError("interactive terminal test failed: %s" % error)
    print("Interactive terminal exchanged input and output.")


def run_test(args, iteration):
    print("Sending request to compile and run a C++ program through the daemon at %s" % args.url)
    request_response = call(
        args.url, "request", request_params(args.interactive), "daemon-execution-request-%d" % iteration)
    adminticket = request_response["adminticket"]
    monitorticket = request_response["monitorticket"]
    print("Received adminticket: %s" % adminticket)
    if args.interactive:
        asyncio.run(run_interactive_test(
            args.url, monitorticket, request_response["executionticket"], args.timeout))
        return

    monitor_messages = asyncio.run(
        monitor_execution(args.url, monitorticket, args.timeout))
    result = call(args.url, "getresult", {
        "adminticket": adminticket,
        "pluginversion": 2021052513,
    }, "daemon-execution-result-%d" % iteration)
    if "message:compilation" not in monitor_messages or "retrieve:" not in monitor_messages:
        raise RuntimeError("monitor did not report compilation and result retrieval")
    if result.get("compilation"):
        raise RuntimeError("compilation failed:\n%s" % result["compilation"])
    if result.get("execution") != EXPECTED_OUTPUT:
        raise RuntimeError("unexpected execution output: %r" % result.get("execution"))
    print("Daemon compiled and ran the test program.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", nargs='?', default='http://127.0.0.1:8880/', help="daemon JSON-RPC URL")
    parser.add_argument("--timeout", type=int, default=5, help="maximum result wait in seconds")
    parser.add_argument("--iterations", type=int, default=1, help="number of compile/run iterations")
    parser.add_argument("--interactive", action="store_true", help="test terminal input/output over the execution WebSocket")
    args = parser.parse_args()
    if args.iterations < 1:
        parser.error("--iterations must be at least 1")
    for iteration in range(1, args.iterations + 1):
        if args.iterations > 1:
            print("Iteration %d of %d" % (iteration, args.iterations))
        run_test(args, iteration)
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as error:
        print("Daemon execution test failed: %s" % error, file=sys.stderr)
        sys.exit(1)
