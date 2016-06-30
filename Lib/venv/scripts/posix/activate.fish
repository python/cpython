# This file must be used with ". bin/activate.fish" *from fish* (http://fishshell.org)
# you cannot run it directly

function deactivate  -d "Exit virtualenv and return to normal shell environment"
    # reset old environment variables
    if test -n "$_OLD_VIRTUAL_PATH"
        set -gx PATH $_OLD_VIRTUAL_PATH
        set -e _OLD_VIRTUAL_PATH
    end
    if test -n "$_OLD_VIRTUAL_PYTHONHOME"
        set -gx PYTHONHOME $_OLD_VIRTUAL_PYTHONHOME
        set -e _OLD_VIRTUAL_PYTHONHOME
    end

    if test -n "$_OLD_FISH_PROMPT_OVERRIDE"
        functions -e fish_prompt
        set -e _OLD_FISH_PROMPT_OVERRIDE
        . ( begin
                printf "function fish_prompt\n\t#"
                functions _old_fish_prompt
            end | psub )
        functions -e _old_fish_prompt
    end

    set -e VIRTUAL_ENV
    if test "$argv[1]" != "nondestructive"
        # Self destruct!
        functions -e deactivate
    end
end

# unset irrelavent variables
deactivate nondestructive

set -gx VIRTUAL_ENV "__VENV_DIR__"

set -gx _OLD_VIRTUAL_PATH $PATH
set -gx PATH "$VIRTUAL_ENV/__VENV_BIN_NAME__" $PATH

# unset PYTHONHOME if set
if set -q PYTHONHOME
    set -gx _OLD_VIRTUAL_PYTHONHOME $PYTHONHOME
    set -e PYTHONHOME
end

if test -z "$VIRTUAL_ENV_DISABLE_PROMPT"
    # fish uses a function instead of an env var to generate the prompt.

    # save the current fish_prompt function as the function _old_fish_prompt
    . ( begin
            printf "function _old_fish_prompt\n\t#"
            functions fish_prompt
        end | psub )

    # with the original prompt function renamed, we can override with our own.
    function fish_prompt
        # Prompt override?
        if test -n "__VENV_PROMPT__"
            printf "%s%s%s" "__VENV_PROMPT__" (set_color normal) (_old_fish_prompt)
            return
        end
        # ...Otherwise, prepend env
        set -l _checkbase (basename "$VIRTUAL_ENV")
        if test $_checkbase = "__"
            # special case for Aspen magic directories
            # see http://www.zetadev.com/software/aspen/
            printf "%s[%s]%s %s" (set_color -b blue white) (basename (dirname "$VIRTUAL_ENV")) (set_color normal) (_old_fish_prompt)
        else
            printf "%s(%s)%s%s" (set_color -b blue white) (basename "$VIRTUAL_ENV") (set_color normal) (_old_fish_prompt)
        end
    end

    set -gx _OLD_FISH_PROMPT_OVERRIDE "$VIRTUAL_ENV"
end
