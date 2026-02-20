"""
Render junit reports as HTML
"""
import os
import importlib.util
from typing import TYPE_CHECKING

if TYPE_CHECKING: # pragma: no cover
    from os import PathLike
    from typing import Optional, List, Dict

from jinja2 import Environment, PackageLoader, select_autoescape, FileSystemLoader, BaseLoader


class LoaderFactory:
    """Obtain jinja template loaders"""

    DEFAULT = "default"
    pkgname = "junit2htmlreport"

    THEME_DIR_ENV = "JUNIT2HTML_THEMES"
    THEME_FILES = ["base.html", "matrix.html", "report.html", "styles.css"]

    def __init__(self):
        self.baseloaders: "Dict[str, BaseLoader]" = {
            self.DEFAULT: PackageLoader(self.pkgname, os.path.join("templates", self.DEFAULT)),
        }

        spec = importlib.util.find_spec(self.pkgname)
        if spec and spec.origin:
            pkgdir = os.path.dirname(spec.origin)
            templates_dir = os.path.join(pkgdir, "templates")
            for name in os.listdir(templates_dir):
                if name not in self.baseloaders.keys():
                    self.baseloaders[name] = PackageLoader(self.pkgname, os.path.join("templates", name))

        extra_themes = os.getenv(self.THEME_DIR_ENV, None)
        if extra_themes and os.path.isdir(extra_themes):
            for root, folders, filenames in os.walk(os.path.abspath(extra_themes)):
                if filenames:
                    theme = True
                    for item in self.THEME_FILES:
                        if item not in filenames:
                            theme = False
                    if theme:
                        theme_name = os.path.basename(root)
                        self.baseloaders[theme_name] = FileSystemLoader(root)

        self.selected = self.DEFAULT

    def add_filesystem_loader(self, name: str, path: "PathLike"):
        self.baseloaders[name] = FileSystemLoader(path)

    def select_loader(self, name: str) -> None:
        if name in self.baseloaders.keys():
            self.selected = name

    def get_loader(self) -> BaseLoader:
        return self.baseloaders[self.selected]

    @property
    def styles(self) -> list:
        return sorted(list(self.baseloaders.keys()))


class JunitLoaderBase:
    """Contains a jinja template BaseLoader"""
    def __init__(self):
        self._loader_factory: "Optional[LoaderFactory]" = None

    @property
    def loader_factory(self) -> "LoaderFactory":
        if self._loader_factory is None:
            return LoaderFactory()
        return self._loader_factory

    @loader_factory.setter
    def loader_factory(self, value):
        self._loader_factory = value


class ReportContainer(JunitLoaderBase):
    """
    Hold one or more reports
    """
    reports: "Dict[str, Junit]"

    def __init__(self):
        super().__init__()
        self.reports = {}

    def add_report(self, filename: str) -> None:
        raise NotImplementedError()

    def failures(self):
        """
        Return all the failed test cases
        :return:
        """
        found: "List[Case]" = []
        for report in self.reports:
            for suite in self.reports[report].suites:
                found.extend(suite.failed())

        return found

    def skips(self):
        """
        Return all the skipped test cases
        :return:
        """
        found: "List[Case]" = []
        for report in self.reports:
            for suite in self.reports[report].suites:
                found.extend(suite.skipped())
        return found




class HTMLReport:

    def __init__(self, show_toc: bool=True):
        self.show_toc = show_toc
        self.title: str = ""
        self.report: "Optional[Junit]" = None

    def load(self, report: "Junit", title: str="JUnit2HTML Report"):
        self.report = report
        self.title = title

    def __iter__(self):
        if self.report is None:
            raise Exception("A report must be loaded through `load(...)` first.")

        return self.report.__iter__()

    def __str__(self) -> str:
        loader = self.report.loader_factory.get_loader()
        env = Environment(loader=loader, autoescape=select_autoescape(["html"]))
        template = env.get_template("report.html")
        return template.render(report=self, title=self.title, show_toc=self.show_toc)

