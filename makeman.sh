#!/bin/bash

#    make manuals automatically, generate man pages from procedure - makeman.sh 
#    Copyright (C) 2022  lazypwny751
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.

# TODO:
# function push;
#       sends the created manuals to man's path.

# adam yapılmaz, adam doğulmaz, adam olunur, adam ölünür.
#                                               --lazypwny751

# Variables
export CWD="${PWD}" TAB="$(printf '\t')"
export status="true" section="1" version="1.8.0" SETUPTEMPLATE="disable" PROJECT="mydoc"
export requirements=(
    "md2roff"
    "mkdir"
    "file"
    "gzip"
    "date"
    "man"
    "cat"
)

# Functions
makeman:templatem() {
    if [[ "${#}" -ge 1 ]] ; then
        case "${1}" in
            1|2|3|4|5|6|7|8|9)
                cat <<KEDI
# ${PROJECT} ${1} $(date +%Y-%m-%d) "1.0.0" 

## NAME

## SYNOPSIS

## DESCRIPTION

## OPTIONS

## BUGS

## COPYRIGHT

## AUTHOR

## SEE ALSO
KEDI
            ;;
            *)
                echo -e "\t${FUNCNAME##*:}: unkonw option."
                return 2
            ;;
        esac
    else
        echo -e "\t${FUNCNAME##*:}: insufficient staff."
        return 1
    fi
}

# Pre-Check
for i in ${requirements[@]} ; do
    if ! command -v "${i}" &> /dev/null ; then
        echo -e "\t${0##*/}: i require '${i}', but you don't have it.."
        export status="false"
    fi
done

if [[ "${status}" = "false" ]] ; then
    exit 1
fi

# Get Arg&Options
while [[ "${#}" -gt 0 ]] ; do
    case "${1}" in
        --[pP][rR][oO][jJ][eE][cC][tT]|-[pP][rR])
            shift
            if [[ -n "${1}" ]] ; then
                export PROJECT="${1}"
                shift
            fi
        ;;
        --[sS][aA][vV][eE]|-[sS][aA])
            shift
            if [[ -d "${1}" ]] ; then
                export SAVEDIR="${1}"
                shift
            fi
        ;;
        --[sS][eE][cC][tT][iI][oO][nN]|-[sS][eE])
            shift
            if [[ -n "${1}" ]] ; then
                case "${1}" in
                    2|3|4|5|6|7|8|9)
                        export section="${1}"
                    ;;
                    *)
                        export section="1"
                    ;;
                esac
                shift
            fi
        ;;
        --[mM][aA][rR][kK][dD][oO][wW][nN]|-[mM][dD])
            shift
            if (file "${1}" | grep -w "ASCII text") &> /dev/null ; then
                export MARKDOWN="${1}"
                shift
            else
                echo -e "\t${0##*/}: please give me MARKDOWN file."
                exit 1
            fi 
        ;;
        --[tT][eE][mM][pP][lL][aA][tT][eE]|-[tT][eE])
            export SETUPTEMPLATE="enable"
            shift
        ;;
        --[vV][eE][rR][sS][iI][oO][nN]|-[vV])
            export SETMODE="version"
            shift
        ;;
        --[hH][eE][lL][pP]|-[hH])
            export SETMODE="help"
            shift
        ;;
        *)
            shift
        ;;
    esac
done

# Main
set -e

if [[ "${SETMODE}" = "version" ]] ; then
    echo "${version}"
elif [[ "${SETMODE}" = "help" ]] ; then
    cat <<HELP
${0##*/} v${version}

There are 7 flags for ${0##*/}:
${TAB}--project, -pr <parameter>
${TAB}${TAB}sets the project name.
${TAB}--save, -sa <parameter>
${TAB}${TAB}sets the directory where the work will be saved.
${TAB}--section, -se <parameter>
${TAB}${TAB}This is the manual's standard definition of section. It needs a parameter from 1 to 9.
${TAB}--markdown, -md <parameter>
${TAB}${TAB}path to your markdown file.
${TAB}--template, -te
${TAB}${TAB}enable the template mod, it create 'doc/${PROJECT}.${section}.md'.
${TAB}--version, -v
${TAB}${TAB}print current version of ${0##*/}.
${TAB}--help, -h
${TAB}${TAB}show's this screen.

22.07.2022, pnm team - lazypwny751
HELP
else
    if [[ "${SETUPTEMPLATE}" = "enable" ]] ; then
        if [[ -n "${SAVEDIR}" ]] ; then
            if [[ ! -f "${SAVEDIR}/doc/${PROJECT}.${section}.md" ]] ; then
                mkdir -p "${SAVEDIR}/doc"
                makeman:templatem "${section}" > "${SAVEDIR}/doc/${PROJECT}.${section}.md"
            else
                echo -e "\t${0##*/}: '${PROJECT}.${section}.md' is already exist, please be careful of your documents."
                exit 1
            fi
        else
            if [[ -d "${CWD}/doc" ]] ; then
                if [[ ! -f "${CWD}/doc/${PROJECT}.${section}.md" ]] ; then
                    makeman:templatem "${section}" > "${CWD}/doc/${PROJECT}.${section}.md"
                else
                    echo -e "\t${0##*/}: '${PROJECT}.${section}.md' is already exist, please be careful of your documents."
                    exit 1 
                fi
            else
                mkdir -p "${CWD}/doc"
                makeman:templatem "${section}" > "${CWD}/doc/${PROJECT}.${section}.md"
            fi
        fi
    else
        if [[ -n "${MARKDOWN}" ]] ; then
            if [[ -n "${SAVEDIR}" ]]; then
                md2roff "${CWD}/${MARKDOWN}" > "${CWD}/${MARKDOWN##*/}.${section}"
                gzip -c "${CWD}/${MARKDOWN##*/}.${section}" > "${SAVEDIR}/${PROJECT}.man.${section}"
                rm -rf "${CWD}/${MARKDOWN##*/}.${section}"
            else
                md2roff "${CWD}/${MARKDOWN}" > "${CWD}/${MARKDOWN##*/}.${section}"
                gzip -c "${CWD}/${MARKDOWN##*/}.${section}" > "${CWD}/${PROJECT}.man.${section}"
                rm -rf "${CWD}/${MARKDOWN##*/}.${section}"
            fi
        else
            echo -e "wrong usage, please type 'bash ${0##*/} --help' for more information."
            exit 1
        fi
    fi
fi
