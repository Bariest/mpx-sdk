"""mpx-cli entry point — argument parsing and command dispatch."""

from __future__ import annotations

import argparse
import sys

from mpx_cli.commands.init import add_init_parser, cmd_init
from mpx_cli.commands.build import add_build_parser, cmd_build
from mpx_cli.commands.upload import add_upload_parser, cmd_upload
from mpx_cli.commands.run import add_run_parser, cmd_run
from mpx_cli.commands.list_skills import add_list_parser, cmd_list
from mpx_cli.commands.delete import add_delete_parser, cmd_delete
from mpx_cli.commands.auth import add_auth_parser, cmd_signup, cmd_login, cmd_logout
from mpx_cli.commands.search import add_search_parser, cmd_search
from mpx_cli.commands.info import add_info_parser, cmd_info
from mpx_cli.commands.versions import add_versions_parser, cmd_versions
from mpx_cli.commands.publish import add_publish_parser, cmd_publish
from mpx_cli.commands.robot import add_robot_parser, cmd_robot
from mpx_cli.sdk.connection import DEFAULT_HOST, DEFAULT_PORT
from mpx_cli.sdk.gateway import DEFAULT_GATEWAY_URL


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="mpx-cli",
        description="MPX-Dog Skill Development Kit — build & deploy WASM skills",
    )
    parser.add_argument(
        "--ip", "-i",
        default=DEFAULT_HOST,
        help=f"Robot IP address (default: {DEFAULT_HOST})",
    )
    parser.add_argument(
        "--port", "-p",
        type=int,
        default=DEFAULT_PORT,
        help=f"Robot HTTP port (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--gateway",
        default=DEFAULT_GATEWAY_URL,
        help=f"Marketplace gateway URL (default: {DEFAULT_GATEWAY_URL}, env: MPX_GATEWAY_URL)",
    )
    parser.add_argument(
        "--version",
        action="version",
        version=f"mpx-cli {__import__('mpx_cli').__version__}",
    )

    sub = parser.add_subparsers(dest="command", required=True)

    # Local robot commands (offline, no gateway needed)
    add_init_parser(sub)
    add_build_parser(sub)
    add_upload_parser(sub)
    add_run_parser(sub)
    add_list_parser(sub)
    add_delete_parser(sub)

    # Marketplace commands (require gateway)
    add_auth_parser(sub)
    add_publish_parser(sub)
    add_search_parser(sub)
    add_info_parser(sub)
    add_versions_parser(sub)
    add_robot_parser(sub)

    return parser


def main(argv: list[str] | None = None) -> None:
    parser = build_parser()
    args = parser.parse_args(argv)

    # Inject gateway URL for cloud commands that need it
    if hasattr(args, "gateway") and args.gateway:
        args.gateway_url = args.gateway

    commands = {
        "init": cmd_init,
        "build": cmd_build,
        "upload": cmd_upload,
        "run": cmd_run,
        "list": cmd_list,
        "delete": cmd_delete,
        "signup": cmd_signup,
        "login": cmd_login,
        "logout": cmd_logout,
        "publish": cmd_publish,
        "search": cmd_search,
        "info": cmd_info,
        "versions": cmd_versions,
        "robot": cmd_robot,
    }

    try:
        commands[args.command](args)
    except Exception as e:
        print(f"❌ {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()