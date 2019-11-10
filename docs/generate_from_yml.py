#!/usr/bin/env python3
"""
This module generates the documentation pages in docs/source/_generated from
the yml files in docs/yml.
"""
import bs4
import datetime
import os
import re
import sys
import yaml

from bs4 import BeautifulSoup

_COPYRIGHT_NOTICE = """.. Copyright (c) 2019, J. D. Mitchell

   Distributed under the terms of the GPL license version 3.

   The full license is in the file LICENSE, distributed with this software.

   This file was auto-generated by docs/generate_from_yml.py, do not edit.
"""

_RST_FILES_WRITTEN = {}

# TODO include brief description in overview page
# TODO include _ignore to ignore certain symbols
# TODO figure out how to handle overloads (it's currently rather manual)
# TODO erase files in _generated if there is no longer a corresponding yml file
# or entry


def _warn(fname, msg):
    assert isinstance(msg, str)
    sys.stderr.write("WARNING in %s: %s\n" % (fname, msg))


def _info(msg, fname=None):
    assert isinstance(msg, str)
    if fname is not None:
        sys.stdout.write("\033[1m%s: %s\033[0m\n" % (fname, msg))
    else:
        sys.stdout.write("\033[1m%s\033[0m\n" % msg)


def _time_since_epoch_to_human(n):
    return datetime.datetime.fromtimestamp(n).strftime("%Y-%m-%d %H:%M:%S.%f")


def run_doxygen():
    return True
    if not os.path.exists("build/xml"):
        _info("Running doxygen, build/xml does not exist")
        return True
    last_changed_source = 0
    for f in os.listdir("../include"):
        if not f.startswith("."):
            f = os.path.join("..", "include", f)
            if os.path.isfile(f) and f.endswith(".hpp"):
                last_changed_source = max(
                    os.path.getmtime(f), last_changed_source
                )
    _info(
        "Last modified file in include/*.hpp      "
        + _time_since_epoch_to_human(last_changed_source)
    )
    first_built_file = float("inf")
    for root, dirs, files in os.walk("build/html/_generated"):
        for f in files:
            f = os.path.join(root, f)
            first_built_file = min(os.path.getmtime(f), first_built_file)
    _info(
        "First generated file in docs/build/html  "
        + _time_since_epoch_to_human(first_built_file)
    )
    return first_built_file < last_changed_source


def convert_cpp_name_to_fname(func):
    p = re.compile(r"operator\s*\*")
    func = p.sub("operator_star", func)
    p = re.compile(r"operator!=")
    func = p.sub("operator_not_eq", func)
    p = re.compile(r"operator\(\)")
    func = p.sub("call_operator", func)
    p = re.compile(r"operator<$")
    func = p.sub("operator_less", func)
    p = re.compile(r"operator\<\<")
    func = p.sub("insertion_operator", func)
    p = re.compile(r"operator==")
    func = p.sub("operator_equal_to", func)
    p = re.compile(r"operator\>")
    func = p.sub("operator_greater", func)
    p = re.compile(r"[\W]")
    func = p.sub("_", func)
    return func.lower()
    # FIXME FroidurePin::Degree is mistakenly documented as FroidurePin::degree
    # p = re.compile(r'([A-Z])')
    # return p.sub(r'_\1', func).lower()


def strip_prefix(func, nr=1):
    return "::".join(func.split("::")[nr:])


def rst_function_source_filename(class_name, func):
    return os.path.join(
        "source/_generated",
        convert_cpp_name_to_fname(class_name + "::" + func) + ".rst",
    )


def rst_function_toc_filename(func):
    return convert_cpp_name_to_fname(func)


def rst_overview_filename(name):
    return os.path.join(
        "source/_generated", convert_cpp_name_to_fname(name) + ".rst"
    )


def rst_doxygen(typename, classname, funcname=None):
    if funcname is not None:
        return """\n.. doxygen%s:: %s::%s
   :project: libsemigroups\n""" % (
            typename,
            classname,
            funcname,
        )
    else:
        return """\n.. doxygen%s:: %s
   :project: libsemigroups\n""" % (
            typename,
            classname,
        )


def rst_toc_tree_stub(name):
    return """\n.. toctree::
       :maxdepth: 2\n\n"""


def rst_cppfunc(classname, name):
    return (
        "       " + rst_function_toc_filename(classname + "::" + name) + "\n"
    )


def rst_section(name):
    underline = "=" * len(name)
    return "\n" + name + "\n" + underline + "\n"


def rst_subsection(name):
    underline = "-" * len(name)
    return "\n" + name + "\n" + underline + "\n"


def rebuild_overview_rst_file(ymlfname, rstfname):
    """Only rewrite the overview rst file in docs/_generated if it doesn't
    exist or the yml file has been modified more recently."""
    return (not os.path.isfile(rstfname)) or (
        os.path.getmtime(ymlfname) > os.path.getmtime(rstfname)
        or os.path.getmtime("generate_from_yml.py")
        > os.path.getmtime(ymlfname)
        or os.path.getmtime("generate_from_yml.py")
        > os.path.getmtime(rstfname)
    )


def rebuild_function_rst_file(ymlfname, rstfname):
    """Only rewrite the function rst file in docs/_generated if it doesn't
    exist."""
    return (not (os.path.exists(rstfname) and os.path.isfile(rstfname))) or (
        os.path.getmtime("generate_from_yml.py") > os.path.getmtime(ymlfname)
        or os.path.getmtime("generate_from_yml.py")
        > os.path.getmtime(rstfname)
    )


def generate_functions_rst(ymlfname):
    with open(ymlfname, "r") as f:
        ymldic = yaml.load(f, Loader=yaml.FullLoader)
        classname = next(iter(ymldic))
        if ymldic[classname] is None:
            return
        for sectiondic in ymldic[classname]:
            name = next(iter(sectiondic))
            if name[0] != "_" and sectiondic[name] is not None:
                for func in sectiondic[name]:
                    if not isinstance(func, list):
                        tnam = "function"
                    else:
                        assert len(func) == 2
                        tnam = func[0]
                        func = func[1]
                    rstfname = rst_function_source_filename(classname, func)
                    _RST_FILES_WRITTEN[rstfname] = True
                    if not rebuild_function_rst_file(ymlfname, rstfname):
                        continue
                    else:
                        _info('Rebuilding %s ...' % rstfname)
                    out = _COPYRIGHT_NOTICE + rst_section(func)
                    out += rst_doxygen(tnam, classname, func)
                    with open(rstfname, "w") as f:
                        f.write(out)


def generate_overview_rst(ymlfname):
    with open(ymlfname, "r") as f:
        out = _COPYRIGHT_NOTICE
        ymldic = yaml.load(f, Loader=yaml.FullLoader)
        onam = next(iter(ymldic))  # object name
        _RST_FILES_WRITTEN[rst_overview_filename(onam)] = True
        if not rebuild_overview_rst_file(
            ymlfname, rst_overview_filename(onam)
        ):
            return
        else:
            _info('Rebuilding %s ...' % rst_overview_filename(onam))
        header_written = False
        out += rst_section(onam[onam.find("::") + 2 :])
        if ymldic[onam] is not None:
            for sectdic in ymldic[onam]:
                name = next(iter(sectdic))
                if name == "_kind":
                    header_written = True
                    out += rst_doxygen(sectdic["_kind"], onam)
                elif header_written is False:
                    header_written = True
                    out += rst_doxygen("class", onam)
                if name[0] != "_":
                    out += rst_subsection(name)
                    out += rst_toc_tree_stub(name)
                    if sectdic[name] is None:
                        continue
                    for func in sorted(sectdic[name]):
                        if isinstance(func, list):
                            func = func[1]
                        out += rst_cppfunc(onam, func)
        with open(rst_overview_filename(onam), "w") as f:
            f.write(out)


def get_doxygen_filename(name):
    p = re.compile(r"::")
    name = p.sub("_1_1", name)
    p = re.compile(r"([A-Z])")
    name = p.sub(r"_\1", name).lower()
    if os.path.isfile("build/xml/class" + name + ".xml"):
        return "build/xml/class" + name + ".xml"
    elif os.path.isfile("build/xml/struct" + name + ".xml"):
        return "build/xml/struct" + name + ".xml"


def xml_ignore(n):
    assert isinstance(n, bs4.element.Tag)
    name = xml_get_unqualified_name(n)
    return (
        name[0] == ":"
        or (name[0].isupper() and name != xml_get_class_name(n))
        or name[0] == "~"
    )


def xml_get_kind(n):
    assert isinstance(n, bs4.element.Tag)
    assert len(n.find_all("kind")) == 1
    return n.find_all("kind")[0].get_text()


def xml_get_scope(n):
    assert isinstance(n, bs4.element.Tag)
    assert len(n.find_all("scope")) == 1
    return n.find_all("scope")[0].get_text()


def xml_get_unqualified_name(n):
    assert isinstance(n, bs4.element.Tag)
    assert len(n.find_all("name")) == 1
    return n.find_all("name")[0].get_text()


def xml_get_class_name(n):
    assert isinstance(n, bs4.element.Tag)
    scope = xml_get_scope(n)
    pos = scope.rfind("::")
    return None if pos == -1 else scope[pos + 2 :]


def xml_get_qualified_name(n):
    assert isinstance(n, bs4.element.Tag)
    assert len(n.find_all("name")) == 1
    assert len(n.find_all("scope")) == 1
    if not xml_ignore(n):
        name = xml_get_unqualified_name(n)
        return xml_get_scope(n) + "::" + name


def check_public_mem_fns_doc(fname):
    with open(fname, "r") as f:
        ymldic = yaml.load(f, Loader=yaml.FullLoader)
        classname = next(iter(ymldic))
        try:
            xml = open(get_doxygen_filename(classname), "r")
        except:
            _warn(fname, "no Doxygen xml file found for:\n" + classname)
            return

        mem_fn = BeautifulSoup(xml, "xml")
        mem_fn = mem_fn.find_all("member")
        mem_fn = [x for x in mem_fn if x.attrs["prot"] == "public"]
        # mem_fn = [x for x in mem_fn if x.attrs['kind'] == 'function']
        mem_fn = [xml_get_qualified_name(x) for x in mem_fn]
        mem_fn = sorted(list(set([x for x in mem_fn if x is not None])))
        mem_fn_doc = []
        if ymldic[classname] is not None:
            for sectiondic in ymldic[classname]:
                name = next(iter(sectiondic))
                if name[0] != "_":
                    if sectiondic[name] is None:
                        continue
                    for fn in sectiondic[name]:
                        if isinstance(fn, list):
                            fn = fn[1]
                        pos = fn.find("(")
                        if pos != -1:
                            fn = fn[:pos]
                        fn = classname + "::" + fn
                        if fn not in mem_fn:
                            _warn(
                                fname,
                                "no Doxygen xml found for:\n" + " " * 9 + fn,
                            )
                        else:
                            mem_fn_doc.append(fn)
        mem_fn_doc = sorted(list(set(mem_fn_doc)))
        missing = ""
        for fn in mem_fn:
            if fn not in mem_fn_doc:
                missing += "- " + fn + "\n"
        if len(missing) > 0:
            _warn(fname, "undocumented member functions:\n" + missing)


def clean_up():
    """Removes files in docs/source/_generated that were not generated by this
    script."""
    for fname in os.listdir("source/_generated"):
        fname = os.path.join("source/_generated", fname)
        if fname not in _RST_FILES_WRITTEN:
            _info(fname, "deleting!!!")
            os.remove(fname)


def main():
    if sys.version_info[0] < 3:
        raise Exception("Python 3 is required")
    try:
        os.mkdir("source/_generated")
    except FileExistsError:
        pass
    if run_doxygen():
        _info("Running doxygen!")
        os.system("doxygen")
    else:
        _info("Not running doxygen!")
    for fname in os.listdir("yml"):
        if fname[0] != ".":
            fname = os.path.join("yml", fname)
            generate_functions_rst(fname)
            generate_overview_rst(fname)
            check_public_mem_fns_doc(fname)
    clean_up()


if __name__ == "__main__":
    main()
