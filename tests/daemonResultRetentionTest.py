#!/usr/bin/env python3
"""Check completed batch results survive monitor closure until retrieval."""

import argparse
import asyncio

from daemonExecutionTest import call, monitor_url, request_params, websockets


async def run_test(url, close_message=False, delay=1, compilation_error=False, no_monitor=False, expire=False, legacy_running=False):
    if expire:
        no_monitor = True
    print("SCENARIO: %s; %s; %s; %s" % (
        "compilation failure" if compilation_error else "execution result",
        "no monitor" if no_monitor else "client close message" if close_message else "WebSocket close frame",
        "expiry with repeated polling" if expire else "retrieval after %ss" % delay,
        "legacy running request" if legacy_running else "standard pluginversion"), flush=True)
    params = request_params()
    params["files"] = {"compile.sh": (
        "#!/bin/sh\necho compile-error\nexit 1\n" if compilation_error else
        "#!/bin/sh\nprintf '#!/bin/sh\\necho retention-ok\\n' > vpl_execution\n"
        "chmod +x vpl_execution\n")}
    params["fileencoding"] = {"compile.sh": 0}
    task = call(url, "request", params, "retention-request")
    admin = {"adminticket": task["adminticket"], "pluginversion": 2021052513}
    running_params = {"adminticket": task["adminticket"]} if legacy_running else admin
    websocket = None
    try:
        if no_monitor:
            await asyncio.sleep(2)
        else:
            websocket = await websockets.connect(monitor_url(url, task["monitorticket"]))
            while True:
                message = await asyncio.wait_for(websocket.recv(), 10)
                if message == "retrieve:":
                    break
            if close_message:
                await websocket.send("close:")
            await websocket.close()
        if expire:
            # Polling must not refresh the harvest timeout.
            for poll in range(8):
                await asyncio.sleep(4)
                try:
                    alive = call(url, "running", running_params, "retention-running")["running"]
                except RuntimeError as error:
                    if not legacy_running or "HTTP Error 500" not in str(error):
                        raise
                    alive = False
                if poll < 6:
                    assert alive, "result disappeared before the harvest timeout"
            assert not alive, "polling extended result retention"
            print("PASS: result expired despite repeated polling")
            return
        await asyncio.sleep(delay)
        assert call(url, "running", running_params, "retention-running")["running"], "completed result was discarded"
        result = call(url, "getresult", admin, "retention-result")
        if compilation_error:
            assert "compile-error" in result["compilation"], result
        else:
            assert result["execution"] == "retention-ok\n", result
        print("PASS: %s, %s, delay %ss" % (
            "compilation failure" if compilation_error else "execution result",
            "no monitor" if no_monitor else "client close message" if close_message else "WebSocket close frame", delay))
    finally:
        if websocket is not None:
            await websocket.close()
        call(url, "stop", admin, "retention-stop")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", nargs="?", default="http://127.0.0.1:8880/")
    parser.add_argument("--close-message", action="store_true")
    parser.add_argument("--no-monitor", action="store_true")
    parser.add_argument("--compilation-error", action="store_true")
    parser.add_argument("--delay", type=float, default=1)
    parser.add_argument("--expire", action="store_true", help="check the 30-second harvest timeout without a monitor, with repeated polling")
    parser.add_argument("--legacy-running", action="store_true", help="omit pluginversion only from running requests")
    args = parser.parse_args()
    if websockets is None:
        parser.error("the Python websockets package is required")
    asyncio.run(run_test(args.url, args.close_message, args.delay, args.compilation_error, args.no_monitor, args.expire, args.legacy_running))


if __name__ == "__main__":
    main()
