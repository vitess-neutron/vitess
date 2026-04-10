# -*- coding: utf-8 -*-

import sys
from pathlib import Path
from typing import Any, List, Optional, Tuple, Union

import matplotlib.pyplot as plt
import numpy as np


class MonitorFile:
    """
    A Monitor output file.

    Attributes:
        x: X values
        y: Y values (2-dimensional data only)
        value: measured intensity
        error: absolute error estimate (optional)
        events: number of events (optional)
        title: title from the file header
        x_label: x_label from the file header
        y_label: y_label from the file header
        x_range: x_range from the file header
        y_range: y_range from the file header
        dimension: 2 or 3
    """

    path: Path
    title: str
    x_label: str
    y_label: str
    x_range: Optional[Tuple[float, float]]
    y_range: Optional[Tuple[float, float]]
    dimension: int
    comments: List[str]
    x: np.ndarray
    y: Optional[np.ndarray]
    value: np.ndarray
    error: Optional[np.ndarray]
    events: Optional[np.ndarray]

    def __init__(self, path: Union[str, Path]):
        self.path = Path(path)
        self.title = ""
        self.x_label = ""
        self.y_label = ""
        self.x_range = None
        self.y_range = None
        self.dimension = 0
        self.comments = list()
        self.y = None
        self.error = None
        self.events = None
        with self.path.open("r") as f:
            skip = -1
            for line in f:
                skip += 1
                line = line.strip()
                if not line:
                    continue
                if not line.startswith("#"):
                    firstline = np.loadtxt([line])
                    if len(firstline) > 5:  # matrix format
                        self.x = firstline
                        skip += 1
                    break
                line = line[1:]
                for part in line.split("#"):  # old files with bugged header
                    part = part.strip()
                    if not part:
                        continue
                    key, value = part.split(":", 1)
                    key = key.strip()
                    value = value.strip()
                    if key.lower() == "title":
                        self.title = value
                    elif key.lower() == "x_label":
                        self.x_label = value
                    elif key.lower() == "y_label":
                        self.y_label = value
                    elif key.lower() == "x_range":
                        lo, hi = value.split(",")
                        self.x_range = (float(lo), float(hi))
                    elif key.lower() == "y_range":
                        lo, hi = value.split(",")
                        self.y_range = (float(lo), float(hi))
                    else:
                        self.comments.append(part)
            f.seek(0)
            data = np.loadtxt(f, skiprows=skip)
            if len(data.shape) < 2:
                # matrix format, one line
                self.dimension = 2
                self.y = data[0:1]
                self.value = data[1:].reshape((1, self.x.shape[0]))
                if (
                    len(self.x) != self.value.shape[1]
                    or len(self.y) != self.value.shape[0]
                ):
                    raise ValueError("Could not parse Matrix format")
            elif data.shape[1] == 4:
                # 1D XYZ
                self.dimension = 1
                self.x = data[:, 0]
                self.value = data[:, 1]
                self.error = data[:, 2]
                self.events = data[:, 3]
            elif data.shape[1] == 5:
                # 2D XYZ
                self.dimension = 2
                self.x = np.sort(np.unique(data[:, 0]))
                nx = len(self.x)
                self.y = np.sort(np.unique(data[:, 1]))
                ny = len(self.y)
                if data.shape[0] != nx * ny:
                    raise ValueError("Could not determine image size")
                ix = np.searchsorted(self.x, data[:, 0], side="left")
                iy = np.searchsorted(self.y, data[:, 1], side="left")
                self.value = np.zeros((ny, nx))
                self.value[(iy, ix)] = data[:, 2]
                self.error = np.zeros((ny, nx))
                self.error[(iy, ix)] = data[:, 3]
                self.events = np.zeros((ny, nx))
                self.events[(iy, ix)] = data[:, 4]
            elif data.shape[1] > 5:
                # 2D matrix
                self.dimension = 2
                self.y = data[:, 0]
                self.value = data[:, 1:]
                if (
                    len(self.x) != self.value.shape[1]
                    or len(self.y) != self.value.shape[0]
                ):
                    raise ValueError("Could not parse Matrix format")

    def plot(self, **kwargs: Any) -> Any:
        """
        Plot the data using matplotlib.

        1-dimensional: line plot with errorbar (errorbar)
        2-dimensional: color mesh (pcolormesh)

        Parameters:
            show: immidiately show the figure
            axes: (optional) matplotlib axes
            figure: (optional) matplotlib figure
            **kwargs: additional parameters for the matplotlib function

        Returns:
            the Figure
        """
        show = kwargs.pop("show", False)
        axes = kwargs.pop("axes", None)
        if axes is None:
            fig = kwargs.pop("figure", None)
            if fig is None:
                fig = plt.figure()
            axes = fig.gca()
        if self.title:
            axes.set_title(self.title + " " + self.path.name)
        else:
            axes.set_title(self.path.name)
        if self.x_label:
            axes.set_xlabel(self.x_label)
        if self.y_label:
            axes.set_ylabel(self.y_label)
        if self.x_range:
            axes.set_xlim(self.x_range[0], self.x_range[1])
        if self.y_range:
            axes.set_ylim(self.y_range[0], self.y_range[1])
        if self.dimension == 1:
            width = (self.x[1] - self.x[0]) * 0.8
            if len(self.x) > 200:
                # avoid barplot for large files
                plt.plot(self.x, self.value, **kwargs)
            else:
                plt.bar(self.x, self.value, width=width, **kwargs)
                if self.error is not None:
                    plt.bar(
                        self.x,
                        self.error * 2,
                        bottom=self.value - self.error,
                        width=width,
                        color="black",
                        alpha=0.4,
                        **kwargs,
                    )
        else:
            x, y, value = self._spread_2d()
            axes.pcolormesh(x, y, value, **kwargs)
            axes.figure.colorbar(axes.collections[0], ax=axes)
        axes.figure.tight_layout()
        if show:
            plt.show()
        return axes.figure

    def _spread_2d(self) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        """Spread out one-lined data for plotting"""
        if self.value is None or self.y is None:
            raise ValueError("missing values")
        if self.dimension == 2 and self.value.shape[0] == 1:
            if self.y_range:
                y = np.array(self.y_range)
            else:
                y = np.array([self.y[0] - 0.5, self.y[0] + 0.5])
            value = np.vstack((self.value, self.value))
            return self.x, y, value
        return self.x, self.y, self.value

    def show(self, depth: int = 1, indent: int = 0) -> None:
        """
        Pretty print info about the file.

        Parameters:
            depth: if depth >= 1 is given, print the details aswell.
        """
        print("  " * indent + repr(self))
        if depth >= 1:
            pad = "  " * (indent + 1)
            nx = len(self.x)
            if self.error is not None:
                err = "with error estimates"
            else:
                err = "without error estimates"
            if self.dimension == 1:
                print(f"{pad}{nx} points, {err}")
            elif self.dimension == 2:
                assert self.y is not None
                ny = len(self.y)
                print(f"{pad}{nx} x {ny} points, {err}")

    def __repr__(self) -> str:
        return f"{type(self).__qualname__}('{self.path.name}', {self.dimension}D)"


if __name__ == "__main__":
    f = MonitorFile(sys.argv[1])
    f.plot(show=True)
