"""
Functions to return rotation matrices.

These are the functions to find the rotation of any point around an axis by an
angle in degrees.

Functions
---------
xRot:
    Return the rotation matrix for a rotation about the x axis.
yRot:
    Return the rotation matrix for a rotation about the y axis.
zRot:
    Return the rotation matrix for a rotation about the z axis.
"""
from math import radians, cos, sin
import numpy as np


def xRot(theta: float) -> np.ndarray:
    """
    Return the rotation matrix for a rotation about the x axis.

    Parameters
    ----------
    theta: float
        An angle in degrees to rotate about the x axis.

    Returns
    -------
    mat: np.ndarray
        3x3 rotation matrix array.

    Notes
    -----
    The input angle is converted to radians before being used.
    """
    angle = radians(theta)
    mat = np.array([[1, 0, 0],
                   [0, cos(angle), sin(angle)],
                   [0, -sin(angle), cos(angle)]])
    return mat


def yRot(theta: float) -> np.ndarray:
    """
    Return the rotation matrix for a rotation about the y axis.

    Parameters
    ----------
    theta: float
        An angle in degrees to rotate about the y axis.

    Returns
    -------
    mat: np.ndarray
        3x3 rotation matrix array.

    Notes
    -----
    The input angle is converted to radians before being used.
    """
    angle = radians(theta)
    mat = np.array([[cos(angle), 0, sin(angle)],
                   [0, 1, 0],
                   [-sin(angle), 0, cos(angle)]])
    return mat


def zRot(theta: float) -> np.ndarray:
    """
    Return the rotation matrix for a rotation about the z axis.

    Parameters
    ----------
    theta: float
        An angle in degrees to rotate about the z axis.

    Returns
    -------
    mat: np.ndarray
        3x3 rotation matrix array.

    Notes
    -----
    The input angle is converted to radians before being used.
    """
    angle = radians(theta)
    mat = np.array([[cos(angle), sin(angle), 0],
                   [-sin(angle), cos(angle), 0],
                   [0, 0, 1]])
    return mat
