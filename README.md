# VXPP

VWPP is a C++ library that wraps VxWorks concurrency primitives with type- and exception-safe objects. These classes manage resources using modern C++ idioms. This library is documented at https://cdcvs.fnal.gov/redmine/projects/vxpp/wiki.

## Changes from 2.6:

- Use C++ namespaces to enforce, at load time, that the correct version has been loaded.

- Improve the `VME::Memory` templates. The generated code now prevents the compiler from optimizing across memory accesses.

## System Preparation

VWPP has been built for every supported target under VxWorks 6.4 and 6.7. If you run across any incompatibilities, please let us know. The project is being managed under Redmine (https://cdcvs.fnal.gov/redmine/projects/vxpp). You can report problems using the issue tracker.

### Location

Our development environment supports several combinations of hardware and software platforms. Be sure to download from the appropriate area.

|   BSP  | VxWorks | File |
| ------ | ------- | ---- |
| mv2400 | 6.4 | vxworks_boot/v6.4/module/mv2400/vwpp-2.7.out |
| mv2401 | 6.4 | vxworks_boot/v6.4/module/mv2401/vwpp-2.7.out |
| mv2434 | 6.4 | vxworks_boot/v6.4/module/mv2434/vwpp-2.7.out |
| mv5500 | 6.4 | vxworks_boot/v6.4/module/mv5500/vwpp-2.7.out |
| mv2400 | 6.7 | vxworks_boot/v6.7/module/mv2400/vwpp-2.7.out |
| mv2401 | 6.7 | vxworks_boot/v6.7/module/mv2401/vwpp-2.7.out |
| mv2434 | 6.7 | vxworks_boot/v6.7/module/mv2434/vwpp-2.7.out |
| mv5500 | 6.7 | vxworks_boot/v6.7/module/mv5500/vwpp-2.7.out |

Rather than load this module, one could also link their module with the static library. For instance, to build a module called `demo.out`, a `Makefile` might define the rule as:

    demo.out : demo.o ${PRODUCTS_LIBDIR}/libvwpp-2.7.a
            ${make-mod}
