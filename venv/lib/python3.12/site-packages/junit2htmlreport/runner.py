"""
Small command line tool to generate a html version of a junit report file
"""
from typing import TYPE_CHECKING

import os
import sys
from argparse import ArgumentParser

from . import matrix, merge, parser, render

if TYPE_CHECKING:
    from typing import List

loader_factory = render.LoaderFactory()
installed_themes = sorted(list(loader_factory.styles))

PARSER = ArgumentParser(prog="junit2html")

PARSER.add_argument("--summary-matrix", dest="text_matrix", action="store_true",
                    default=False,
                    help="Render multiple result files to the console")

PARSER.add_argument("--report-matrix", dest="html_matrix", type=str,
                    metavar="REPORT",
                    help="Generate an HTML report matrix")

PARSER.add_argument("--max-failures", dest="fail", type=int, default=0,
                    metavar="FAILURES",
                    help="Exit non-zero if FAILURES or more test cases are failures (has no effect with --merge)")

PARSER.add_argument("--max-skipped", dest="skip", type=int, default=0,
                    metavar="SKIPPED",
                    help="Exit non-zero if SKIPPED or more test cases are skipped (has no effect with --merged)")

PARSER.add_argument("--merge", dest="merge_output", type=str,
                    metavar="NEWREPORT",
                    help="Merge multiple test results into one file")
THEME_MUTEX = PARSER.add_mutually_exclusive_group()
THEME_MUTEX.add_argument("--theme", type=str, dest="theme", choices=installed_themes,
                    help="Use an installed junit2html theme")

THEME_MUTEX.add_argument("--theme-dir", type=str, dest="template_folder",
                    help="Use a folder containing custom html and CSS.")

PARSER.add_argument("--reports-template-folder", dest="template_folder", type=str,
                    help="Render reports with these templates (deprecated, use '--theme-dir' instead")

PARSER.add_argument("--hide-toc", dest="hide_toc", action="store_true",
                    default=False,
                    help="Don't include a table-of-contents in the HTML report")

PARSER.add_argument("REPORTS", metavar="REPORT", type=str, nargs="+",
                    help="Test file to read")

PARSER.add_argument("OUTPUT", type=str, nargs="?",
                    help="Filename to save the html as")


def run(args: "List[str]"):
    """
    Run this tool
    :param args:
    :return:
    """
    opts = PARSER.parse_args(args) if args else PARSER.parse_args()
    if opts.template_folder and os.path.isdir(opts.template_folder):
        loader_factory.add_filesystem_loader(loader_factory.DEFAULT, os.path.abspath(opts.template_folder))
    if opts.theme:
        loader_factory.select_loader(opts.theme)
    inputs = opts.REPORTS
    util = None
    if opts.merge_output:
        util = merge.Merger()
        for inputfile in inputs:
            util.add_report(inputfile)

        xmltext = util.toxmlstring()
        with open(opts.merge_output, "w") as outfile:
            outfile.write(xmltext)
    elif opts.text_matrix:
        util = matrix.TextReportMatrix()
        for filename in inputs:
            util.add_report(filename)
        print(util.summary())
    elif opts.html_matrix:
        util = matrix.HtmlReportMatrix(os.path.dirname(opts.html_matrix))
        util.loader_factory = loader_factory
        for filename in inputs:
            util.add_report(filename, show_toc=not opts.hide_toc)
        with open(opts.html_matrix, "w") as outfile:
            outfile.write(util.summary())

    if util:
        if opts.fail:
            failed = util.failures()
            if len(failed) >= opts.fail:
                sys.exit(len(failed))
        if opts.skip:
            skipped = util.skips()
            if len(skipped) >= opts.fail:
                sys.exit(len(skipped))

    if not util:
        # legacy interface that we need to preserve
        # no options, one or two args, first is input file, optional second is output

        if len(opts.REPORTS) > 2:
            PARSER.print_usage()
            sys.exit(1)

        infilename = opts.REPORTS[0]

        if len(opts.REPORTS) == 2:
            outfilename = opts.REPORTS[1]
        else:
            outfilename = infilename + ".html"

        report = parser.Junit(infilename)
        report.loader_factory = loader_factory
        html = report.html(show_toc=not opts.hide_toc)
        if report.filename is not None:
            with open(outfilename, "wb") as outfile:
                outfile.write(html.encode('utf-8'))
        else:
            print(html.encode('utf-8'))


def start():
    """
    Run using the current sys.argv
    """
    run(sys.argv[1:])


if __name__ == "__main__":
    start()
