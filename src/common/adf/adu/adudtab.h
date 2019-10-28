/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf -m 50 -F',"",0,0' adudtab.txt  */
/* Computed positions: -k'1-5,7,9,12,14' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 138 "adudtab.txt"

static unsigned int adu_dttemplate_hash (
	register const char *str,
	register unsigned int len);

const struct _ANSIDATETMPLT *adu_dttemplate_lookup (
	register const char *str,
	register unsigned int len);
#line 148 "adudtab.txt"
struct _ANSIDATETMPLT;
/* maximum key range = 199, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
adu_dttemplate_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203,   7,   7, 203, 203, 203, 203, 203, 203,
      203, 203, 203,   8, 203,   0,   7,  11,   2, 203,
      203, 203, 203, 203, 203, 203, 203, 203,   0,   0,
      203, 203, 203, 203, 203,   0, 203, 203,  88, 203,
       79,  63, 203,  62, 203, 203, 203,  89,  59, 203,
      203, 203, 203, 203, 203,  17, 203, 203,  11,   1,
      203, 203, 203, 203, 203,  27,  27, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203,  92,
        2,  43, 203, 203,  94, 203, 203, 203, 203, 203,
      203, 203,  81, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203, 203, 203, 203,
      203, 203, 203, 203, 203, 203, 203
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[13]];
      /*FALLTHROUGH*/
      case 13:
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      /*FALLTHROUGH*/
      case 11:
      case 10:
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]+1];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct _ANSIDATETMPLT *
adu_dttemplate_lookup (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 123,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 17,
      MIN_HASH_VALUE = 4,
      MAX_HASH_VALUE = 202
    };

  static const struct _ANSIDATETMPLT wordlist[] =
    {
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
#line 172 "adudtab.txt"
      {"An",			"y",		DB_INYM_TYPE,	DT_I },
#line 275 "adudtab.txt"
      {"Yn",		"Y", DB_DTE_TYPE, DT_CHK},
      {"","",0,0}, {"","",0,0},
#line 163 "adudtab.txt"
      {"An:n",			"hi",		DB_TMWO_TYPE,	DT_T },
      {"","",0,0},
#line 164 "adudtab.txt"
      {"An:n:n",			"his",		DB_TMWO_TYPE,	DT_T },
#line 273 "adudtab.txt"
      {"Yn/n",		"md", DB_DTE_TYPE, DT_0},
#line 169 "adudtab.txt"
      {"An:n-n:n",		"hicv",		DB_TMW_TYPE,	DT_T },
      {"","",0,0},
#line 170 "adudtab.txt"
      {"An:n:n-n:n",		"hiscv",	DB_TMW_TYPE,	DT_T },
#line 173 "adudtab.txt"
      {"An-n",			"yq",		DB_INYM_TYPE,	DT_I },
#line 274 "adudtab.txt"
      {"Yn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 156 "adudtab.txt"
      {"An-n-n",			"ymd",		DB_ADTE_TYPE,	DT_0 },
#line 271 "adudtab.txt"
      {"Yn-n-n",		"ymd", DB_DTE_TYPE, DT_CHK},
#line 165 "adudtab.txt"
      {"An:n:n.n",		"hisl",		DB_TMWO_TYPE,	DT_T },
#line 166 "adudtab.txt"
      {"An:n+n:n",		"hibv",		DB_TMW_TYPE,	DT_T },
#line 202 "adudtab.txt"
      {"Un",		"K", DB_DTE_TYPE, DT_CHK},
#line 167 "adudtab.txt"
      {"An:n:n+n:n",		"hisbv",	DB_TMW_TYPE,	DT_T },
      {"","",0,0},
#line 268 "adudtab.txt"
      {"Yn/n/n",		"ymd", DB_DTE_TYPE, DT_0},
#line 171 "adudtab.txt"
      {"An:n:n.n-n:n",		"hislcv",	DB_TMW_TYPE,	DT_T },
#line 157 "adudtab.txt"
      {"An-n-nn:n:n",		"ymdhis",	DB_TSWO_TYPE,	DT_T},
#line 200 "adudtab.txt"
      {"Un/n",		"md", DB_DTE_TYPE, DT_0},
#line 311 "adudtab.txt"
      {"Xn-n-n",		"ymd", DB_DTE_TYPE, DT_0},
#line 272 "adudtab.txt"
      {"Yn.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 159 "adudtab.txt"
      {"An-n-nn:n:n-n:n",	"ymdhiscv",	DB_TSW_TYPE,	DT_T },
      {"","",0,0},
#line 201 "adudtab.txt"
      {"Un-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 168 "adudtab.txt"
      {"An:n:n.n+n:n",		"hislbv",	DB_TMW_TYPE,	DT_T },
#line 198 "adudtab.txt"
      {"Un-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 158 "adudtab.txt"
      {"An-n-nn:n:n.n",		"ymdhisl",	DB_TSWO_TYPE,	DT_T},
      {"","",0,0}, {"","",0,0},
#line 161 "adudtab.txt"
      {"An-n-nn:n:n+n:n",	"ymdhisbv",	DB_TSW_TYPE,	DT_T },
#line 160 "adudtab.txt"
      {"An-n-nn:n:n.n-n:n",	"ymdhislcv",	DB_TSW_TYPE,	DT_T },
#line 195 "adudtab.txt"
      {"Un/n/n",		"mdy", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
#line 199 "adudtab.txt"
      {"Un.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 176 "adudtab.txt"
      {"A-n",			"x",		DB_INYM_TYPE,	DT_I },
#line 162 "adudtab.txt"
      {"An-n-nn:n:n.n+n:n",	"ymdhislbv",	DB_TSW_TYPE,	DT_T },
#line 178 "adudtab.txt"
      {"Ann",			"dh",		DB_INDS_TYPE,	DT_I|DT_T},
#line 186 "adudtab.txt"
      {"A-nn",			"eh",		DB_INDS_TYPE,	DT_I|DT_T},
#line 177 "adudtab.txt"
      {"A-n-n",			"xq",		DB_INYM_TYPE,	DT_I },
#line 187 "adudtab.txt"
      {"A-nn:n",			"ehi",		DB_INDS_TYPE,	DT_I|DT_T},
#line 179 "adudtab.txt"
      {"Ann:n",			"dhi",		DB_INDS_TYPE,	DT_I|DT_T},
#line 188 "adudtab.txt"
      {"A-nn:n:n",		"ehis",		DB_INDS_TYPE,	DT_I|DT_T},
#line 174 "adudtab.txt"
      {"A+n",			"y",		DB_INYM_TYPE,	DT_I },
      {"","",0,0},
#line 180 "adudtab.txt"
      {"Ann:n:n",		"dhis",		DB_INDS_TYPE,	DT_I|DT_T},
#line 182 "adudtab.txt"
      {"A+nn",			"dh",		DB_INDS_TYPE,	DT_I|DT_T},
#line 175 "adudtab.txt"
      {"A+n-n",			"yq",		DB_INYM_TYPE,	DT_I },
#line 183 "adudtab.txt"
      {"A+nn:n",			"dhi",		DB_INDS_TYPE,	DT_I|DT_T},
#line 181 "adudtab.txt"
      {"Ann:n:n.n",		"dhisl",	DB_INDS_TYPE,	DT_I|DT_T},
#line 184 "adudtab.txt"
      {"A+nn:n:n",		"dhis",		DB_INDS_TYPE,	DT_I|DT_T},
#line 189 "adudtab.txt"
      {"A-nn:n:n.n",		"ehisl",	DB_INDS_TYPE,	DT_I|DT_T},
#line 216 "adudtab.txt"
      {"Nn",		"K", DB_DTE_TYPE, DT_CHK},
      {"","",0,0},
#line 267 "adudtab.txt"
      {"Yn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 245 "adudtab.txt"
      {"In",		"J", DB_DTE_TYPE, DT_CHK},
#line 260 "adudtab.txt"
      {"Gn",		"G", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 214 "adudtab.txt"
      {"Nn/n",		"md", DB_DTE_TYPE, DT_0},
#line 185 "adudtab.txt"
      {"A+nn:n:n.n",		"dhisl",	DB_INDS_TYPE,	DT_I|DT_T},
      {"","",0,0},
#line 243 "adudtab.txt"
      {"In/n",		"md", DB_DTE_TYPE, DT_0},
#line 258 "adudtab.txt"
      {"Gn/n",		"md", DB_DTE_TYPE, DT_0},
#line 215 "adudtab.txt"
      {"Nn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 310 "adudtab.txt"
      {"Xn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 212 "adudtab.txt"
      {"Nn-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 244 "adudtab.txt"
      {"In-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 259 "adudtab.txt"
      {"Gn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 241 "adudtab.txt"
      {"In-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 256 "adudtab.txt"
      {"Gn-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 194 "adudtab.txt"
      {"Un_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 209 "adudtab.txt"
      {"Nn/n/n",		"dmy", DB_DTE_TYPE, DT_0},
#line 230 "adudtab.txt"
      {"Fn",		"K", DB_DTE_TYPE, DT_CHK},
#line 276 "adudtab.txt"
      {"Yz",		"f", DB_DTE_TYPE, DT_0},
#line 238 "adudtab.txt"
      {"In/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 253 "adudtab.txt"
      {"Gn/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 213 "adudtab.txt"
      {"Nn.n.n",		"ymd", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 228 "adudtab.txt"
      {"Fn/n",		"md", DB_DTE_TYPE, DT_0},
#line 242 "adudtab.txt"
      {"In.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 257 "adudtab.txt"
      {"Gn.n.n",		"dmy", DB_DTE_TYPE, DT_0},
#line 304 "adudtab.txt"
      {"Dn",		"D", DB_DTE_TYPE, DT_CHK},
#line 289 "adudtab.txt"
      {"Mn",		"M", DB_DTE_TYPE, DT_CHK},
#line 229 "adudtab.txt"
      {"Fn-n",		"md", DB_DTE_TYPE, DT_TYP},
      {"","",0,0},
#line 226 "adudtab.txt"
      {"Fn-n-n",		"ymd", DB_DTE_TYPE, DT_CHK},
#line 277 "adudtab.txt"
      {"Yr",		"r", DB_DTE_TYPE, DT_0},
#line 302 "adudtab.txt"
      {"Dn/n",		"dm", DB_DTE_TYPE, DT_0},
#line 287 "adudtab.txt"
      {"Mn/n",		"md", DB_DTE_TYPE, DT_0},
#line 203 "adudtab.txt"
      {"Uz",		"f", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 223 "adudtab.txt"
      {"Fn/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 303 "adudtab.txt"
      {"Dn-n",		"dm", DB_DTE_TYPE, DT_TYP},
#line 288 "adudtab.txt"
      {"Mn-n",		"md", DB_DTE_TYPE, DT_TYP},
#line 300 "adudtab.txt"
      {"Dn-n-n",		"dmy", DB_DTE_TYPE, DT_CHK},
#line 285 "adudtab.txt"
      {"Mn-n-n",		"mdy", DB_DTE_TYPE, DT_CHK},
#line 227 "adudtab.txt"
      {"Fn.n.n",		"ymd", DB_DTE_TYPE, DT_0},
#line 270 "adudtab.txt"
      {"Yn-m-n",		"ymd", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0},
#line 297 "adudtab.txt"
      {"Dn/n/n",		"dmy", DB_DTE_TYPE, DT_0},
#line 283 "adudtab.txt"
      {"Mn/n/n",		"mdy", DB_DTE_TYPE, DT_0},
#line 204 "adudtab.txt"
      {"Ur",		"r", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 269 "adudtab.txt"
      {"Yn m n",		"ymd", DB_DTE_TYPE, DT_0},
#line 301 "adudtab.txt"
      {"Dn.n.n",		"dmy", DB_DTE_TYPE, DT_0},
#line 286 "adudtab.txt"
      {"Mn.n.n",		"mdy", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
      {"","",0,0},
#line 208 "adudtab.txt"
      {"Nn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 197 "adudtab.txt"
      {"Un-m-n",		"dmy", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 237 "adudtab.txt"
      {"In_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 252 "adudtab.txt"
      {"Gn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0},
#line 196 "adudtab.txt"
      {"Un m n",		"dmy", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
      {"","",0,0}, {"","",0,0},
#line 217 "adudtab.txt"
      {"Nz",		"f", DB_DTE_TYPE, DT_0},
#line 222 "adudtab.txt"
      {"Fn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 246 "adudtab.txt"
      {"Iz",		"f", DB_DTE_TYPE, DT_0},
#line 261 "adudtab.txt"
      {"Gz",		"f", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
      {"","",0,0},
#line 296 "adudtab.txt"
      {"Dn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
#line 282 "adudtab.txt"
      {"Mn_n_n",		"ymd", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 218 "adudtab.txt"
      {"Nr",		"r", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0},
#line 247 "adudtab.txt"
      {"Ir",		"r", DB_DTE_TYPE, DT_0},
#line 262 "adudtab.txt"
      {"Gr",		"r", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0},
#line 231 "adudtab.txt"
      {"Fz",		"f", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0},
#line 211 "adudtab.txt"
      {"Nn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0},
#line 240 "adudtab.txt"
      {"In-m-n",		"dmy", DB_DTE_TYPE, DT_0},
#line 255 "adudtab.txt"
      {"Gn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
#line 305 "adudtab.txt"
      {"Dz",		"f", DB_DTE_TYPE, DT_0},
#line 290 "adudtab.txt"
      {"Mz",		"f", DB_DTE_TYPE, DT_0},
#line 210 "adudtab.txt"
      {"Nn m n",		"dmy", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 232 "adudtab.txt"
      {"Fr",		"r", DB_DTE_TYPE, DT_0},
#line 239 "adudtab.txt"
      {"In m n",		"dmy", DB_DTE_TYPE, DT_0},
#line 254 "adudtab.txt"
      {"Gn m n",		"dmy", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
      {"","",0,0}, {"","",0,0},
#line 306 "adudtab.txt"
      {"Dr",		"r", DB_DTE_TYPE, DT_0},
#line 291 "adudtab.txt"
      {"Mr",		"r", DB_DTE_TYPE, DT_0},
#line 225 "adudtab.txt"
      {"Fn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
      {"","",0,0}, {"","",0,0},
#line 224 "adudtab.txt"
      {"Fn m n",		"dmy", DB_DTE_TYPE, DT_0},
      {"","",0,0},
#line 299 "adudtab.txt"
      {"Dn-m-n",		"dmy", DB_DTE_TYPE, DT_0},
#line 284 "adudtab.txt"
      {"Mm-n-n",		"mdy", DB_DTE_TYPE, DT_0},
      {"","",0,0}, {"","",0,0}, {"","",0,0}, {"","",0,0},
      {"","",0,0},
#line 298 "adudtab.txt"
      {"Dn m n",		"dmy", DB_DTE_TYPE, DT_0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = adu_dttemplate_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].dt_tmplt;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 312 "adudtab.txt"

