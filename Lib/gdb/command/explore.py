# GDB 'explore' command.
# Copyright (C) 2012-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Implementation of the GDB 'explore' command using the GDB Python API."""

import gdb


class Explorer(object):
    """Internal class which invokes other explorers."""

    # This map is filled by the Explorer.init_env() function
    type_code_to_explorer_map = {}

    _SCALAR_TYPE_LIST = (
        gdb.TYPE_CODE_CHAR,
        gdb.TYPE_CODE_INT,
        gdb.TYPE_CODE_BOOL,
        gdb.TYPE_CODE_FLT,
        gdb.TYPE_CODE_VOID,
        gdb.TYPE_CODE_ENUM,
    )

    @staticmethod
    def guard_expr(expr):
        length = len(expr)
        guard = False

        if expr[0] == "(" and expr[length - 1] == ")":
            pass
        else:
            i = 0
            while i < length:
                c = expr[i]
                if (
                    c == "_"
                    or ("a" <= c and c <= "z")
                    or ("A" <= c and c <= "Z")
                    or ("0" <= c and c <= "9")
                ):
                    pass
                else:
                    guard = True
                    break
                i += 1

        if guard:
            return "(" + expr + ")"
        else:
            return expr

    @staticmethod
    def explore_expr(expr, value, is_child):
        """Main function to explore an expression value.

        Arguments:
            expr: The expression string that is being explored.
            value: The gdb.Value value of the expression.
            is_child: Boolean value to indicate if the expression is a child.
                      An expression is a child if it is derived from the main
                      expression entered by the user.  For example, if the user
                      entered an expression which evaluates to a struct, then
                      when exploring the fields of the struct, is_child is set
                      to True internally.

        Returns:
            No return value.
        """
        type_code = value.type.code
        if type_code in Explorer.type_code_to_explorer_map:
            explorer_class = Explorer.type_code_to_explorer_map[type_code]
            while explorer_class.explore_expr(expr, value, is_child):
                pass
        else:
            print("Explorer for type '%s' not yet available.\n" % str(value.type))

    @staticmethod
    def explore_type(name, datatype, is_child):
        """Main function to explore a data type.

        Arguments:
            name: The string representing the path to the data type being
                  explored.
            datatype: The gdb.Type value of the data type being explored.
            is_child: Boolean value to indicate if the name is a child.
                      A name is a child if it is derived from the main name
                      entered by the user.  For example, if the user entered
                      the name of struct type, then when exploring the fields
                      of the struct, is_child is set to True internally.

        Returns:
            No return value.
        """
        type_code = datatype.code
        if type_code in Explorer.type_code_to_explorer_map:
            explorer_class = Explorer.type_code_to_explorer_map[type_code]
            while explorer_class.explore_type(name, datatype, is_child):
                pass
        else:
            print("Explorer for type '%s' not yet available.\n" % str(datatype))

    @staticmethod
    def init_env():
        """Initializes the Explorer environment.
        This function should be invoked before starting any exploration.  If
        invoked before an exploration, it need not be invoked for subsequent
        explorations.
        """
        Explorer.type_code_to_explorer_map = {
            gdb.TYPE_CODE_CHAR: ScalarExplorer,
            gdb.TYPE_CODE_INT: ScalarExplorer,
            gdb.TYPE_CODE_BOOL: ScalarExplorer,
            gdb.TYPE_CODE_FLT: ScalarExplorer,
            gdb.TYPE_CODE_VOID: ScalarExplorer,
            gdb.TYPE_CODE_ENUM: ScalarExplorer,
            gdb.TYPE_CODE_STRUCT: CompoundExplorer,
            gdb.TYPE_CODE_UNION: CompoundExplorer,
            gdb.TYPE_CODE_PTR: PointerExplorer,
            gdb.TYPE_CODE_REF: ReferenceExplorer,
            gdb.TYPE_CODE_RVALUE_REF: ReferenceExplorer,
            gdb.TYPE_CODE_TYPEDEF: TypedefExplorer,
            gdb.TYPE_CODE_ARRAY: ArrayExplorer,
        }

    @staticmethod
    def is_scalar_type(type):
        """Checks whether a type is a scalar type.
        A type is a scalar type of its type is
            gdb.TYPE_CODE_CHAR or
            gdb.TYPE_CODE_INT or
            gdb.TYPE_CODE_BOOL or
            gdb.TYPE_CODE_FLT or
            gdb.TYPE_CODE_VOID or
            gdb.TYPE_CODE_ENUM.

        Arguments:
            type: The type to be checked.

        Returns:
            'True' if 'type' is a scalar type. 'False' otherwise.
        """
        return type.code in Explorer._SCALAR_TYPE_LIST

    @staticmethod
    def return_to_parent_value():
        """A utility function which prints that the current exploration session
        is returning to the parent value. Useful when exploring values.
        """
        print("\nReturning to parent value...\n")

    @staticmethod
    def return_to_parent_value_prompt():
        """A utility function which prompts the user to press the 'enter' key
        so that the exploration session can shift back to the parent value.
        Useful when exploring values.
        """
        input("\nPress enter to return to parent value: ")

    @staticmethod
    def return_to_enclosing_type():
        """A utility function which prints that the current exploration session
        is returning to the enclosing type.  Useful when exploring types.
        """
        print("\nReturning to enclosing type...\n")

    @staticmethod
    def return_to_enclosing_type_prompt():
        """A utility function which prompts the user to press the 'enter' key
        so that the exploration session can shift back to the enclosing type.
        Useful when exploring types.
        """
        input("\nPress enter to return to enclosing type: ")


class ScalarExplorer(object):
    """Internal class used to explore scalar values."""

    @staticmethod
    def explore_expr(expr, value, is_child):
        """Function to explore scalar values.
        See Explorer.explore_expr and Explorer.is_scalar_type for more
        information.
        """
        print("'%s' is a scalar value of type '%s'." % (expr, value.type))
        print("%s = %s" % (expr, str(value)))

        if is_child:
            Explorer.return_to_parent_value_prompt()
            Explorer.return_to_parent_value()

        return False

    @staticmethod
    def explore_type(name, datatype, is_child):
        """Function to explore scalar types.
        See Explorer.explore_type and Explorer.is_scalar_type for more
        information.
        """
        if datatype.code == gdb.TYPE_CODE_ENUM:
            if is_child:
                print("%s is of an enumerated type '%s'." % (name, str(datatype)))
            else:
                print("'%s' is an enumerated type." % name)
        else:
            if is_child:
                print("%s is of a scalar type '%s'." % (name, str(datatype)))
            else:
                print("'%s' is a scalar type." % name)

        if is_child:
            Explorer.return_to_enclosing_type_prompt()
            Explorer.return_to_enclosing_type()

        return False


class PointerExplorer(object):
    """Internal class used to explore pointer values."""

    @staticmethod
    def explore_expr(expr, value, is_child):
        """Function to explore pointer values.
        See Explorer.explore_expr for more information.
        """
        print(
            "'%s' is a pointer to a value of type '%s'"
            % (expr, str(value.type.target()))
        )
        option = input(
            "Continue exploring it as a pointer to a single " "value [y/n]: "
        )
        if option == "y":
            deref_value = None
            try:
                deref_value = value.dereference()
                str(deref_value)
            except gdb.MemoryError:
                print(
                    "'%s' a pointer pointing to an invalid memory " "location." % expr
                )
                if is_child:
                    Explorer.return_to_parent_value_prompt()
                return False
            Explorer.explore_expr(
                "*%s" % Explorer.guard_expr(expr), deref_value, is_child
            )
            return False

        option = input("Continue exploring it as a pointer to an " "array [y/n]: ")
        if option == "y":
            while True:
                index = 0
                try:
                    index = int(
                        input(
                            "Enter the index of the element you "
                            "want to explore in '%s': " % expr
                        )
                    )
                except ValueError:
                    break
                element_expr = "%s[%d]" % (Explorer.guard_expr(expr), index)
                element = value[index]
                try:
                    str(element)
                except gdb.MemoryError:
                    print("Cannot read value at index %d." % index)
                    continue
                Explorer.explore_expr(element_expr, element, True)
            return False

        if is_child:
            Explorer.return_to_parent_value()
        return False

    @staticmethod
    def explore_type(name, datatype, is_child):
        """Function to explore pointer types.
        See Explorer.explore_type for more information.
        """
        target_type = datatype.target()
        print("\n%s is a pointer to a value of type '%s'." % (name, str(target_type)))

        Explorer.explore_type("the pointee type of %s" % name, target_type, is_child)
        return False


class ReferenceExplorer(object):
    """Internal class used to explore reference (TYPE_CODE_REF) values."""

    @staticmethod
    def explore_expr(expr, value, is_child):
        """Function to explore array values.
        See Explorer.explore_expr for more information.
        """
        referenced_value = value.referenced_value()
        Explorer.explore_expr(expr, referenced_value, is_child)
        return False

    @staticmethod
    def explore_type(name, datatype, is_child):
        """Function to explore pointer types.
        See Explorer.explore_type for more information.
        """
        target_type = datatype.target()
        Explorer.explore_type(name, target_type, is_child)
        return False


class ArrayExplorer(object):
    """Internal class used to explore arrays."""

    @staticmethod
    def explore_expr(expr, value, is_child):
        """Function to explore array values.
        See Explorer.explore_expr for more information.
        """
        target_type = value.type.target()
        print("'%s' is an array of '%s'." % (expr, str(target_type)))
        index = 0
        try:
            index = int(
                input(
                    "Enter the index of the element you want to "
                    "explore in '%s': " % expr
                )
            )
        except ValueError:
            if is_child:
                Explorer.return_to_parent_value()
            return False

        element = None
        try:
            element = value[index]
            str(element)
        except gdb.MemoryError:
            print("Cannot read value at index %d." % index)
            input("Press enter to continue... ")
            return True

        Explorer.explore_expr(
            "%s[%d]" % (Explorer.guard_expr(expr), index), element, True
        )
        return True

    @staticmethod
    def explore_type(name, datatype, is_child):
        """Function to explore array types.
        See Explorer.explore_type for more information.
        """
        target_type = datatype.target()
        print("%s is an array of '%s'." % (name, str(target_type)))

        Explorer.explore_type("the array element of %s" % name, target_type, is_child)
        return False


class CompoundExplorer(object):
    """Internal class used to explore struct, classes and unions."""

    @staticmethod
    def _print_fields(print_list):
        """Internal function which prints the fields of a struct/class/union."""
        max_field_name_length = 0
        for pair in print_list:
            if max_field_name_length < len(pair[0]):
                max_field_name_length = len(pair[0])

        for pair in print_list:
            print("  %*s = %s" % (max_field_name_length, pair[0], pair[1]))

    @staticmethod
    def _get_real_field_count(fields):
        real_field_count = 0
        for field in fields:
            if not field.artificial:
                real_field_count = real_field_count + 1

        return real_field_count

    @staticmethod
    def explore_expr(expr, value, is_child):
        """Function to explore structs/classes and union values.
        See Explorer.explore_expr for more information.
        """
        datatype = value.type
        type_code = datatype.code
        fields = datatype.fields()

        if type_code == gdb.TYPE_CODE_STRUCT:
            type_desc = "struct/class"
        else:
            type_desc = "union"

        if CompoundExplorer._get_real_field_count(fields) == 0:
            print(
                "The value of '%s' is a %s of type '%s' with no fields."
                % (expr, type_desc, str(value.type))
            )
            if is_child:
                Explorer.return_to_parent_value_prompt()
            return False

        print(
            "The value of '%s' is a %s of type '%s' with the following "
            "fields:\n" % (expr, type_desc, str(value.type))
        )

        has_explorable_fields = False
        choice_to_compound_field_map = {}
        current_choice = 0
        print_list = []
        for field in fields:
            if field.artificial:
                continue
            field_full_name = Explorer.guard_expr(expr) + "." + field.name
            if field.is_base_class:
                field_value = value.cast(field.type)
            else:
                field_value = value[field.name]
            literal_value = ""
            if type_code == gdb.TYPE_CODE_UNION:
                literal_value = "<Enter %d to explore this field of type " "'%s'>" % (
                    current_choice,
                    str(field.type),
                )
                has_explorable_fields = True
            else:
                if Explorer.is_scalar_type(field.type):
                    literal_value = "%s .. (Value of type '%s')" % (
                        str(field_value),
                        str(field.type),
                    )
                else:
                    if field.is_base_class:
                        field_desc = "base class"
                    else:
                        field_desc = "field"
                    literal_value = "<Enter %d to explore this %s of type " "'%s'>" % (
                        current_choice,
                        field_desc,
                        str(field.type),
                    )
                    has_explorable_fields = True

            choice_to_compound_field_map[str(current_choice)] = (
                field_full_name,
                field_value,
            )
            current_choice = current_choice + 1

            print_list.append((field.name, literal_value))

        CompoundExplorer._print_fields(print_list)
        print("")

        if has_explorable_fields:
            choice = input("Enter the field number of choice: ")
            if choice in choice_to_compound_field_map:
                Explorer.explore_expr(
                    choice_to_compound_field_map[choice][0],
                    choice_to_compound_field_map[choice][1],
                    True,
                )
                return True
            else:
                if is_child:
                    Explorer.return_to_parent_value()
        else:
            if is_child:
                Explorer.return_to_parent_value_prompt()

        return False

    @staticmethod
    def explore_type(name, datatype, is_child):
        """Function to explore struct/class and union types.
        See Explorer.explore_type for more information.
        """
        type_code = datatype.code
        type_desc = ""
        if type_code == gdb.TYPE_CODE_STRUCT:
            type_desc = "struct/class"
        else:
            type_desc = "union"

        fields = datatype.fields()
        if CompoundExplorer._get_real_field_count(fields) == 0:
            if is_child:
                print(
                    "%s is a %s of type '%s' with no fields."
                    % (name, type_desc, str(datatype))
                )
                Explorer.return_to_enclosing_type_prompt()
            else:
                print("'%s' is a %s with no fields." % (name, type_desc))
            return False

        if is_child:
            print(
                "%s is a %s of type '%s' "
                "with the following fields:\n" % (name, type_desc, str(datatype))
            )
        else:
            print("'%s' is a %s with the following " "fields:\n" % (name, type_desc))

        current_choice = 0
        choice_to_compound_field_map = {}
        print_list = []
        for field in fields:
            if field.artificial:
                continue
            if field.is_base_class:
                field_desc = "base class"
            else:
                field_desc = "field"
            rhs = "<Enter %d to explore this %s of type '%s'>" % (
                current_choice,
                field_desc,
                str(field.type),
            )
            print_list.append((field.name, rhs))
            choice_to_compound_field_map[str(current_choice)] = (
                field.name,
                field.type,
                field_desc,
            )
            current_choice = current_choice + 1

        CompoundExplorer._print_fields(print_list)
        print("")

        if len(choice_to_compound_field_map) > 0:
            choice = input("Enter the field number of choice: ")
            if choice in choice_to_compound_field_map:
                if is_child:
                    new_name = "%s '%s' of %s" % (
                        choice_to_compound_field_map[choice][2],
                        choice_to_compound_field_map[choice][0],
                        name,
                    )
                else:
                    new_name = "%s '%s' of '%s'" % (
                        choice_to_compound_field_map[choice][2],
                        choice_to_compound_field_map[choice][0],
                        name,
                    )
                Explorer.explore_type(
                    new_name, choice_to_compound_field_map[choice][1], True
                )
                return True
            else:
                if is_child:
                    Explorer.return_to_enclosing_type()
        else:
            if is_child:
                Explorer.return_to_enclosing_type_prompt()

        return False


class TypedefExplorer(object):
    """Internal class used to explore values whose type is a typedef."""

    @staticmethod
    def explore_expr(expr, value, is_child):
        """Function to explore typedef values.
        See Explorer.explore_expr for more information.
        """
        actual_type = value.type.strip_typedefs()
        print(
            "The value of '%s' is of type '%s' "
            "which is a typedef of type '%s'"
            % (expr, str(value.type), str(actual_type))
        )

        Explorer.explore_expr(expr, value.cast(actual_type), is_child)
        return False

    @staticmethod
    def explore_type(name, datatype, is_child):
        """Function to explore typedef types.
        See Explorer.explore_type for more information.
        """
        actual_type = datatype.strip_typedefs()
        if is_child:
            print(
                "The type of %s is a typedef of type '%s'." % (name, str(actual_type))
            )
        else:
            print("The type '%s' is a typedef of type '%s'." % (name, str(actual_type)))

        Explorer.explore_type(name, actual_type, is_child)
        return False


class ExploreUtils(object):
    """Internal class which provides utilities for the main command classes."""

    @staticmethod
    def check_args(name, arg_str):
        """Utility to check if adequate number of arguments are passed to an
        explore command.

        Arguments:
            name: The name of the explore command.
            arg_str: The argument string passed to the explore command.

        Returns:
            True if adequate arguments are passed, false otherwise.

        Raises:
            gdb.GdbError if adequate arguments are not passed.
        """
        if len(arg_str) < 1:
            raise gdb.GdbError("ERROR: '%s' requires an argument." % name)
            return False
        else:
            return True

    @staticmethod
    def get_type_from_str(type_str):
        """A utility function to deduce the gdb.Type value from a string
        representing the type.

        Arguments:
            type_str: The type string from which the gdb.Type value should be
                      deduced.

        Returns:
            The deduced gdb.Type value if possible, None otherwise.
        """
        try:
            # Assume the current language to be C/C++ and make a try.
            return gdb.parse_and_eval("(%s *)0" % type_str).type.target()
        except RuntimeError:
            # If assumption of current language to be C/C++ was wrong, then
            # lookup the type using the API.
            try:
                return gdb.lookup_type(type_str)
            except RuntimeError:
                return None

    @staticmethod
    def get_value_from_str(value_str):
        """A utility function to deduce the gdb.Value value from a string
        representing the value.

        Arguments:
            value_str: The value string from which the gdb.Value value should
                       be deduced.

        Returns:
            The deduced gdb.Value value if possible, None otherwise.
        """
        try:
            return gdb.parse_and_eval(value_str)
        except RuntimeError:
            return None


class ExploreCommand(gdb.Command):
    """Explore a value or a type valid in the current context.

    Usage: explore ARG

    - ARG is either a valid expression or a type name.
    - At any stage of exploration, hit the return key (instead of a
    choice, if any) to return to the enclosing type or value."""

    def __init__(self):
        super(ExploreCommand, self).__init__(
            name="explore", command_class=gdb.COMMAND_DATA, prefix=True
        )

    def invoke(self, arg_str, from_tty):
        if ExploreUtils.check_args("explore", arg_str) is False:
            return

        # Check if it is a value
        value = ExploreUtils.get_value_from_str(arg_str)
        if value is not None:
            Explorer.explore_expr(arg_str, value, False)
            return

        # If it is not a value, check if it is a type
        datatype = ExploreUtils.get_type_from_str(arg_str)
        if datatype is not None:
            Explorer.explore_type(arg_str, datatype, False)
            return

        # If it is neither a value nor a type, raise an error.
        raise gdb.GdbError(
            (
                "'%s' neither evaluates to a value nor is a type "
                "in the current context." % arg_str
            )
        )


class ExploreValueCommand(gdb.Command):
    """Explore value of an expression valid in the current context.

    Usage: explore value ARG

    - ARG is a valid expression.
    - At any stage of exploration, hit the return key (instead of a
    choice, if any) to return to the enclosing value."""

    def __init__(self):
        super(ExploreValueCommand, self).__init__(
            name="explore value", command_class=gdb.COMMAND_DATA
        )

    def invoke(self, arg_str, from_tty):
        if ExploreUtils.check_args("explore value", arg_str) is False:
            return

        value = ExploreUtils.get_value_from_str(arg_str)
        if value is None:
            raise gdb.GdbError(
                (
                    " '%s' does not evaluate to a value in the current "
                    "context." % arg_str
                )
            )
            return

        Explorer.explore_expr(arg_str, value, False)


class ExploreTypeCommand(gdb.Command):
    """Explore a type or the type of an expression.

    Usage: explore type ARG

    - ARG is a valid expression or a type name.
    - At any stage of exploration, hit the return key (instead of a
    choice, if any) to return to the enclosing type."""

    def __init__(self):
        super(ExploreTypeCommand, self).__init__(
            name="explore type", command_class=gdb.COMMAND_DATA
        )

    def invoke(self, arg_str, from_tty):
        if ExploreUtils.check_args("explore type", arg_str) is False:
            return

        datatype = ExploreUtils.get_type_from_str(arg_str)
        if datatype is not None:
            Explorer.explore_type(arg_str, datatype, False)
            return

        value = ExploreUtils.get_value_from_str(arg_str)
        if value is not None:
            print("'%s' is of type '%s'." % (arg_str, str(value.type)))
            Explorer.explore_type(str(value.type), value.type, False)
            return

        raise gdb.GdbError(
            ("'%s' is not a type or value in the current " "context." % arg_str)
        )


Explorer.init_env()

ExploreCommand()
ExploreValueCommand()
ExploreTypeCommand()
