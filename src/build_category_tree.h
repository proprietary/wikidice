#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace net_zelcon::wikidice {

using std::string_view_literals::operator""sv;

std::vector<std::string_view> WIKIPEDIA_LANGUAGE_CODES{
    "en"sv,      "fr"sv,      "de"sv,        "es"sv,       "ja"sv,
    "ru"sv,      "pt"sv,      "zh"sv,        "it"sv,       "fa"sv,
    "pl"sv,      "ar"sv,      "nl"sv,        "uk"sv,       "he"sv,
    "id"sv,      "tr"sv,      "cs"sv,        "sv"sv,       "ko"sv,
    "vi"sv,      "fi"sv,      "hu"sv,        "ca"sv,       "simple"sv,
    "th"sv,      "no"sv,      "hi"sv,        "bn"sv,       "el"sv,
    "ro"sv,      "sr"sv,      "da"sv,        "bg"sv,       "eu"sv,
    "az"sv,      "ms"sv,      "uz"sv,        "et"sv,       "sk"sv,
    "hr"sv,      "hy"sv,      "sl"sv,        "lt"sv,       "kk"sv,
    "eo"sv,      "lv"sv,      "ta"sv,        "ur"sv,       "ml"sv,
    "ka"sv,      "be"sv,      "gl"sv,        "sq"sv,       "mk"sv,
    "arz"sv,     "sh"sv,      "ha"sv,        "ceb"sv,      "af"sv,
    "ckb"sv,     "te"sv,      "tl"sv,        "bs"sv,       "la"sv,
    "mr"sv,      "ky"sv,      "is"sv,        "mn"sv,       "my"sv,
    "kn"sv,      "sw"sv,      "nn"sv,        "ast"sv,      "be-tarask"sv,
    "azb"sv,     "pa"sv,      "cy"sv,        "as"sv,       "ne"sv,
    "yo"sv,      "ku"sv,      "oc"sv,        "ga"sv,       "jv"sv,
    "lb"sv,      "sa"sv,      "br"sv,        "tt"sv,       "si"sv,
    "sco"sv,     "tg"sv,      "als"sv,       "fy"sv,       "war"sv,
    "min"sv,     "ba"sv,      "so"sv,        "km"sv,       "or"sv,
    "ig"sv,      "pnb"sv,     "gu"sv,        "rw"sv,       "ce"sv,
    "su"sv,      "an"sv,      "io"sv,        "cv"sv,       "zh-classical"sv,
    "bar"sv,     "bcl"sv,     "lmo"sv,       "ht"sv,       "mg"sv,
    "yi"sv,      "fo"sv,      "am"sv,        "ia"sv,       "ps"sv,
    "scn"sv,     "tk"sv,      "wuu"sv,       "ban"sv,      "qu"sv,
    "co"sv,      "ary"sv,     "mai"sv,       "sat"sv,      "zu"sv,
    "nds"sv,     "pms"sv,     "kaa"sv,       "ace"sv,      "lo"sv,
    "mt"sv,      "bh"sv,      "bjn"sv,       "dag"sv,      "mzn"sv,
    "vec"sv,     "szl"sv,     "li"sv,        "vls"sv,      "sd"sv,
    "vo"sv,      "om"sv,      "sc"sv,        "bo"sv,       "hyw"sv,
    "cr"sv,      "ang"sv,     "tw"sv,        "sah"sv,      "ab"sv,
    "gn"sv,      "hif"sv,     "ie"sv,        "diq"sv,      "mad"sv,
    "frr"sv,     "crh"sv,     "lfn"sv,       "xmf"sv,      "nap"sv,
    "ext"sv,     "ay"sv,      "cdo"sv,       "frp"sv,      "rue"sv,
    "wa"sv,      "nds-nl"sv,  "tly"sv,       "gd"sv,       "tcy"sv,
    "map-bms"sv, "ff"sv,      "gor"sv,       "iu"sv,       "mwl"sv,
    "mi"sv,      "hsb"sv,     "ug"sv,        "guc"sv,      "lad"sv,
    "lij"sv,     "se"sv,      "pcd"sv,       "av"sv,       "kw"sv,
    "dz"sv,      "eml"sv,     "ilo"sv,       "ti"sv,       "chr"sv,
    "ee"sv,      "glk"sv,     "dv"sv,        "mhr"sv,      "cu"sv,
    "tay"sv,     "kbd"sv,     "avk"sv,       "roa-tara"sv, "ks"sv,
    "os"sv,      "bat-smg"sv, "atj"sv,       "bpy"sv,      "bug"sv,
    "gan"sv,     "hak"sv,     "pam"sv,       "gv"sv,       "roa-rup"sv,
    "bxr"sv,     "myv"sv,     "krc"sv,       "kv"sv,       "ln"sv,
    "mni"sv,     "new"sv,     "pap"sv,       "pdc"sv,      "rm"sv,
    "sn"sv,      "vep"sv,     "mrj"sv,       "smn"sv,      "dsb"sv,
    "pih"sv,     "skr"sv,     "st"sv,        "tpi"sv,      "udm"sv,
    "zea"sv,     "bm"sv,      "lld"sv,       "ksh"sv,      "kl"sv,
    "inh"sv,     "kbp"sv,     "kab"sv,       "csb"sv,      "rn"sv,
    "fiu-vro"sv, "wo"sv,      "arc"sv,       "haw"sv,      "ki"sv,
    "kg"sv,      "shi"sv,     "tn"sv,        "tyv"sv,      "fur"sv,
    "gur"sv,     "xal"sv,     "nah"sv,       "nv"sv,       "shn"sv,
    "za"sv,      "ny"sv,      "got"sv,       "koi"sv,      "olo"sv,
    "mnw"sv,     "nia"sv,     "tum"sv,       "ch"sv,       "cbk-zam"sv,
    "gag"sv,     "ltg"sv,     "jbo"sv,       "pcm"sv,      "nrm"sv,
    "pag"sv,     "szy"sv,     "kcg"sv,       "xh"sv,       "ami"sv,
    "fat"sv,     "fon"sv,     "lg"sv,        "nov"sv,      "blk"sv,
    "rmy"sv,     "alt"sv,     "tet"sv,       " anp"sv,     "awa"sv,
    "bi"sv,      "dty"sv,     "gpe"sv,       "lez"sv,      "mdf"sv,
    "nqo"sv,     "sm"sv,      "ty"sv,        "gom"sv,      "ady"sv,
    "fj"sv,      "jam"sv,     "pnt"sv,       "stq"sv,      "srn"sv,
    "pfl"sv,     "din"sv,     "ik"sv,        "lbe"sv,      "nso"sv,
    "pwn"sv,     "ss"sv,      "to"sv,        "ts"sv,       "ve"sv,
    "chy"sv,     "pi"sv,      "sg"sv,        "guw"sv,      "trv"sv,
    "gcr"sv,     "zh-yue"sv,  "zh-min-nan"sv};

auto is_valid_language(std::string_view language) -> bool;

} // namespace net_zelcon::wikidice
