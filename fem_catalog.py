from __future__ import annotations


SUPPORTED_ELEMENT_TYPES = ("tet4", "hex8", "prism6")
SUPPORTED_OPERATORS = (
    "diffusion",
    "mass",
    "convection",
    "diffusion_mass",
    "diffusion_convection_mass",
    "laplace",
    "test",
)


def nshape(element_type: str) -> int:
    et = str(element_type).lower()
    if et == "tet4":
        return 4
    if et == "hex8":
        return 8
    if et == "prism6":
        return 6
    raise ValueError(f"Unsupported element_type: {element_type}")


def qp_cap(element_type: str) -> int:
    et = str(element_type).lower()
    if et == "tet4":
        return 4
    if et == "hex8":
        return 8
    if et == "prism6":
        return 6
    raise ValueError(f"Unsupported element_type: {element_type}")


def flops_per_elem_qp(element_type: str, operator: str) -> float:
    et = str(element_type).lower()
    op = str(operator).lower()
    if et == "tet4":
        table = {
            "diffusion": 330.0,
            "mass": 120.0,
            "convection": 210.0,
            "diffusion_mass": 450.0,
            "diffusion_convection_mass": 660.0,
            "laplace": 330.0,
            "test": 660.0,
        }
    elif et == "hex8":
        table = {
            "diffusion": 1200.0,
            "mass": 420.0,
            "convection": 820.0,
            "diffusion_mass": 1620.0,
            "diffusion_convection_mass": 2440.0,
            "laplace": 1200.0,
            "test": 2440.0,
        }
    elif et == "prism6":
        table = {
            "diffusion": 780.0,
            "mass": 260.0,
            "convection": 500.0,
            "diffusion_mass": 1040.0,
            "diffusion_convection_mass": 1540.0,
            "laplace": 780.0,
            "test": 1820.0,
        }
    else:
        raise ValueError(f"Unsupported element_type: {element_type}")
    if op not in table:
        raise ValueError(f"Unsupported operator: {operator}")
    return table[op]


def bytes_per_elem_qp(element_type: str, dtype: str) -> float:
    itemsize = 4.0 if str(dtype).lower() == "float32" else 8.0
    nsh = float(nshape(element_type))
    return (nsh * 3.0 + nsh * nsh) * itemsize


def operator_elapsed_multiplier(element_type: str, operator: str) -> float:
    et = str(element_type).lower()
    op = str(operator).lower()
    if et == "prism6":
        table = {
            "diffusion": 1.00,
            "mass": 0.84,
            "convection": 1.18,
            "diffusion_mass": 1.28,
            "diffusion_convection_mass": 1.44,
            "laplace": 1.00,
            "test": 1.72,
        }
        if op not in table:
            raise ValueError(f"Unsupported operator: {operator}")
        return table[op]
    return 1.0

