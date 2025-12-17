/*
    Colossal Cave Adventure - pure C port

    This file contains a C implementation of the core game based on
    Anthony C. Hay's C++ recoding of Will Crowther's original FORTRAN IV
    Adventure (see cca.cpp in the same directory).

    This version:
    - Uses only the C standard library (no C++ and no C++ standard library)
    - Embeds the original advdat.77-03-31 data table
    - Implements the adventure engine closely following the C++ version

    This is intentionally a single C translation unit.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ------------------------------------------------------------------------- */
/* Basic helpers                                                             */
/* ------------------------------------------------------------------------- */

static void to_upper_inplace(char *s)
{
    if (!s) return;
    for (; *s; ++s)
        *s = (char)toupper((unsigned char)*s);
}

/* five spaces in A5 format (PDP-10 FORTRAN IV 36-bit integer) */
static const uint_least64_t A5_SPACE = 0201004020100ULL;

/* Return given string (0..5 chars) in PDP-10 FORTRAN IV 36-bit integer A5 format. */
static uint_least64_t as_a5(const char *str)
{
    uint_least64_t result = 0;
    unsigned i = 0;
    size_t len = str ? strlen(str) : 0;

    if (len > 5) {
        fprintf(stderr, "as_a5(): given more than 5 characters\n");
        exit(EXIT_FAILURE);
    }

    while (i < len) {
        result <<= 7;
        result |= (uint_least64_t)(str[i] & 0x7F);
        ++i;
    }
    while (i++ < 5) {
        result <<= 7;
        result |= (uint_least64_t)' ';
    }
    result <<= 1;
    return result;
}

/* Return given A5 format integer as a string of 5 characters (not null-terminated). */
static void a5_to_string(uint_least64_t a, char out[6])
{
    int i;
    a >>= 1;
    for (i = 0; i < 5; ++i) {
        out[i] = (char)((a >> ((4 - i) * 7)) & 0177);
    }
    out[5] = '\0';
}

/* ------------------------------------------------------------------------- */
/* Embedded advdat.77-03-31 data (taken from cca.cpp)                        */
/* ------------------------------------------------------------------------- */

/* The contents of the text file http://www.icynic.com/~don/jerz/advdat.77-03-31 */
static const char advdat_77_03_31[] =
"1\n"
"1    YOU ARE STANDING AT THE END OF A ROAD BEFORE A SMALL BRICK\n"
"1    BUILDING . AROUND YOU IS A FOREST. A SMALL\n"
"1    STREAM FLOWS OUT OF THE BUILDING AND DOWN A GULLY.\n"
"2    YOU HAVE WALKED UP A HILL, STILL IN THE FOREST\n"
"2    THE ROAD NOW SLOPES BACK DOWN THE OTHER SIDE OF THE HILL.\n"
"2    THERE IS A BUILDING IN THE DISTANCE.\n"
"3    YOU ARE INSIDE A BUILDING, A WELL HOUSE FOR A LARGE SPRING.\n"
"4    YOU ARE IN A VALLEY IN THE FOREST BESIDE A STREAM TUMBLING\n"
"4    ALONG A ROCKY BED.\n"
"5    YOU ARE IN OPEN FOREST, WITH A DEEP VALLEY TO ONE SIDE.\n"
"6    YOU ARE IN OPEN FOREST NEAR BOTH A VALLEY AND A ROAD.\n"
"7    AT YOUR FEET ALL THE WATER OF THE STREAM SPLASHES INTO A\n"
"7    2 INCH SLIT IN THE ROCK. DOWNSTREAM THE STREAMBED IS BARE ROCK.\n"
"8    YOU ARE IN A 20 FOOT DEPRESSION FLOORED WITH BARE DIRT. SET INTO\n"
"8    THE DIRT IS A STRONG STEEL GRATE MOUNTED IN CONCRETE. A DRY\n"
"8    STREAMBED LEADS INTO THE DEPRESSION.\n"
"9    YOU ARE IN A SMALL CHAMBER BENEATH A 3X3 STEEL GRATE TO THE\n"
"9    SURFACE. A LOW CRAWL OVER COBBLES LEADS INWARD TO THE WEST.\n"
"10   YOU ARE CRAWLING OVER COBBLES IN A LOW PASSAGE. THERE IS A\n"
"10   DIM LIGHT AT THE EAST END OF THE PASSAGE.\n"
"11   YOU ARE IN A DEBRIS ROOM, FILLED WITH STUFF WASHED IN FROM\n"
"11   THE SURFACE. A LOW WIDE PASSAGE WITH COBBLES BECOMES\n"
"11   PLUGGED WITH MUD AND DEBRIS HERE,BUT AN AWKWARD CANYON\n"
"11   LEADS UPWARD AND WEST.\n"
"11   A NOTE ON THE WALL SAYS 'MAGIC WORD XYZZY'.\n"
"12   YOU ARE IN AN AWKWARD SLOPING EAST/WEST CANYON.\n"
"13   YOU ARE IN A SPLENDID CHAMBER THIRTY FEET HIGH. THE WALLS\n"
"13   ARE FROZEN RIVERS OF ORANGE STONE. AN AWKWARD CANYON AND A\n"
"13   GOOD PASSAGE EXIT FROM EAST AND WEST SIDES OF THE CHAMBER.\n"
"14   AT YOUR FEET IS A SMALL PIT BREATHING TRACES OF WHITE MIST. AN\n"
"14   EAST PASSAGE ENDS HERE EXCEPT FOR A SMALL CRACK LEADING ON.\n"
"15   YOU ARE AT ONE END OF A VAST HALL STRETCHING FORWARD OUT OF\n"
"15   SIGHT TO THE WEST. THERE ARE OPENINGS TO EITHER SIDE. NEARBY, A WIDE\n"
"15   STONE STAIRCASE LEADS DOWNWARD. THE HALL IS FILLED WITH\n"
"15   WISPS OF WHITE MIST SWAYING TO AND FRO ALMOST AS IF ALIVE.\n"
"15   A COLD WIND BLOWS UP THE STAIRCASE. THERE IS A PASSAGE\n"
"15   AT THE TOP OF A DOME BEHIND YOU.\n"
"16   THE CRACK IS FAR TOO SMALL FOR YOU TO FOLLOW.\n"
"17   YOU ARE ON THE EAST BANK OF A FISSURE SLICING CLEAR ACROSS\n"
"17   THE HALL. THE MIST IS QUITE THICK HERE, AND THE FISSURE IS\n"
"17   TOO WIDE TO JUMP.\n"
"18   THIS IS A LOW ROOM WITH A CRUDE NOTE ON THE WALL.\n"
"18   IT SAYS 'YOU WON'T GET IT UP THE STEPS'.\n"
"19   YOU ARE IN THE HALL OF THE MOUNTAIN KING, WITH PASSAGES\n"
"19   OFF IN ALL DIRECTIONS.\n"
"20   YOU ARE AT THE BOTTOM OF THE PIT WITH A BROKEN NECK.\n"
"21   YOU DIDN'T MAKE IT\n"
"22   THE DOME IS UNCLIMBABLE\n"
"23   YOU CAN'T GO IN THROUGH A LOCKED STEEL GRATE!\n"
"24   YOU DON'T FIT DOWN A TWO INCH HOLE!\n"
"25   YOU CAN'T GO THROUGH A LOCKED STEEL GRATE.\n"
"27   YOU ARE ON THE WEST SIDE OF THE FISSURE IN THE HALL OF MISTS.\n"
"28   YOU ARE IN A LOW N/S PASSAGE AT A HOLE IN THE FLOOR.\n"
"28   THE HOLE GOES DOWN TO AN E/W PASSAGE.\n"
"29   YOU ARE IN THE SOUTH SIDE CHAMBER.\n"
"30   YOU ARE IN THE WEST SIDE CHAMBER OF HALL OF MT KING.\n"
"30   A PASSAGE CONTINUES WEST AND UP HERE.\n"
"31   THERE IS NO WAY ACROSS THE FISSURE.\n"
"32   YOU CAN'T GET BY THE SNAKE\n"
"33   YOU ARE IN A LARGE ROOM, WITH A PASSAGE TO THE SOUTH,\n"
"33   A PASSAGE TO THE WEST, AND A WALL OF BROKEN ROCK TO\n"
"33   THE EAST. THERE IS A LARGE 'Y2' ON A ROCK IN ROOMS CENTER.\n"
"34   YOU ARE IN A JUMBLE OF ROCK, WITH CRACKS EVERYWHERE.\n"
"35   YOU ARE AT A WINDOW ON A HUGE PIT, WHICH GOES UP AND\n"
"35   DOWN OUT OF SIGHT. A FLOOR IS INDISTINCTLY VISIBLE\n"
"35   OVER 50 FEET BELOW. DIRECTLY OPPOSITE YOU AND 25 FEET AWAY\n"
"35   THERE IS A SIMILAR WINDOW.\n"
"36   YOU ARE IN A DIRTY BROKEN PASSAGE. TO THE EAST IS A CRAWL.\n"
"36   TO THE WEST IS A LARGE PASSAGE. ABOVE YOU IS A HOLE TO\n"
"36   ANOTHER PASSAGE.\n"
"37   YOU ARE ON THE BRINK OF A SMALL CLEAN CLIMBABLE PIT.\n"
"37   A CRAWL LEADS WEST.\n"
"38   YOU ARE IN THE BOTTOM OF A SMALL PIT WITH A LITTLE\n"
"38   STREAM, WHICH ENTERS AND EXITS THROUGH TINY SLITS.\n"
"39   YOU ARE IN A LARGE ROOM FULL OF DUSTY ROCKS. THERE IS A\n"
"39   BIG HOLE IN THE FLOOR. THERE ARE CRACKS EVERYWHERE, AND\n"
"39   A PASSAGE LEADING EAST.\n"
"40   YOU HAVE CRAWLED THROUGH A VERY LOW WIDE PASSAGE PARALLEL\n"
"40   TO AND NORTH OF THE HALL OF MISTS.\n"
"41   YOU ARE AT THE WEST END OF HALL OF MISTS. A LOW WIDE CRAWL\n"
"41   CONTINUES WEST AND ANOTHER GOES NORTH. TO THE SOUTH IS A\n"
"41   LITTLE PASSAGE 6 FEET OFF THE FLOOR.\n"
"42   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"43   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"44   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"45   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"46   DEAD END\n"
"47   DEAD END\n"
"48   DEAD END\n"
"49   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"50   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"51   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"52   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"53   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"54   DEAD END\n"
"55   YOU ARE IN A MAZE OF TWISTY LITTLE PASSAGES, ALL ALIKE.\n"
"56   DEAD END\n"
"57   YOU ARE ON THE BRINK OF A THIRTY FOOT PIT WITH A MASSIVE\n"
"57   ORANGE COLUMN DOWN ONE WALL. YOU COULD CLIMB DOWN HERE\n"
"57   BUT YOU COULD NOT GET BACK UP. THE MAZE CONTINUES AT THIS\n"
"57   LEVEL.\n"
"58   DEAD END\n"
"59   YOU HAVE CRAWLED THROUGH A VERY LOW WIDE PASSAGE PARALLEL\n"
"59   TO AND NORTH OF THE HALL OF MISTS.\n"
"60   YOU ARE AT THE EAST END OF A VERY LONG HALL APPARENTLY\n"
"60   WITHOUT SIDE CHAMBERS. TO THE EAST A LOW WIDE CRAWL SLANTS\n"
"60   UP. TO THE NORTH A ROUND TWO FOOT HOLE SLANTS DOWN.\n"
"61   YOU ARE AT THE WEST END OF A VERY LONG FEATURELESS HALL.\n"
"62   YOU ARE AT A CROSSOVER OF A HIGH N/S PASSAGE AND A LOW E/W ONE.\n"
"63   DEAD END\n"
"64   YOU ARE AT A COMPLEX JUNCTION. A LOW HANDS AND KNEES\n"
"64   PASSAGE FROM THE NORTH JOINS A HIGHER CRAWL\n"
"64   FROM THE EAST TO MAKE  A WALKING PASSAGE GOING WEST\n"
"64   THERE IS ALSO A LARGE ROOM ABOVE. THE AIR IS DAMP HERE.\n"
"64   A SIGN IN MIDAIR HERE SAYS 'CAVE UNDER CONSTRUCTION BEYOND\n"
"64   THIS POINT. PROCEED AT OWN RISK.'\n"
"65   YOU ARE IN BEDQUILT, A LONG EAST/WEST PASSAGE WITH HOLES EVERYWHERE.\n"
"65   TO EXPLORE AT RANDOM SELECT NORTH, SOUTH, UP, OR DOWN.\n"
"66   YOU ARE IN A ROOM WHOSE WALLS RESEMBLE SWISS CHEESE.\n"
"66   OBVIOUS PASSAGES GO WEST,EAST,NE, AND\n"
"66   NW. PART OF THE ROOM IS OCCUPIED BY A LARGE BEDROCK BLOCK.\n"
"67   YOU ARE IN THE TWOPIT ROOM. THE FLOOR\n"
"67   HERE IS LITTERED WITH THIN ROCK SLABS, WHICH MAKE IT\n"
"67   EASY TO DESCEND THE PITS. THERE IS A PATH HERE BYPASSING\n"
"67   THE PITS TO CONNECT PASSAGES FROM EAST AND WEST.THERE\n"
"67   ARE HOLES ALL OVER, BUT THE ONLY BIG ONE IS ON THE WALL\n"
"67   DIRECTLY OVER THE EAST PIT WHERE YOU CAN'T GET TO IT.\n"
"68   YOU ARE IN A LARGE LOW CIRCULAR CHAMBER WHOSE FLOOR IS AN\n"
"68   IMMENSE SLAB FALLEN FROM THE CEILING(SLAB ROOM). EAST AND\n"
"68   WEST THERE ONCE WERE LARGE PASSAGES, BUT THEY ARE NOW FILLED\n"
"68   WITH BOULDERS. LOW SMALL PASSAGES GO NORTH AND SOUTH, AND THE\n"
"68   SOUTH ONE QUICKLY BENDS WEST AROUND THE BOULDERS.\n"
"69   YOU ARE IN A SECRET NS CANYON ABOVE A LARGE ROOM.\n"
"70   YOU ARE IN A SECRET N/S CANYON ABOVE A SIZABLE PASSAGE.\n"
"71   YOU ARE IN SECRET CANYON AT A JUNCTION OF THREE CANYONS,\n"
"71   BEARING NORTH, SOUTH, AND SE. THE NORTH ONE IS AS TALL\n"
"71   AS THE OTHER TWO COMBINED.\n"
"72   YOU ARE IN A LARGE LOW ROOM. CRAWLS LEAD N, SE, AND SW.\n"
"73   DEAD END CRAWL.\n"
"74   YOU ARE IN SECRET CANYON WHICH HERE RUNS E/W. IT CROSSES OVER\n"
"74   A VERY TIGHT CANYON 15 FEET BELOW. IF YOU GO DOWN YOU MAY\n"
"74   NOT BE ABLE TO GET BACK UP\n"
"75   YOU ARE AT A WIDE PLACE IN A VERY TIGHT N/S CANYON.\n"
"76   THE CANYON HERE BECOMES TO TIGHT TO GO FURTHER SOUTH.\n"
"77   YOU ARE IN A TALL E/W CANYON. A LOW TIGHT CRAWL GOES 3 FEET\n"
"77   NORTH AND SEEMS TO OPEN UP.\n"
"78   THE CANYON RUNS INTO A MASS OF BOULDERS - DEAD END.\n"
"79   THE STREAM FLOWS OUT THROUGH A PAIR OF 1 FOOT DIAMETER SEWER\n"
"79   PIPES. IT WOULD BE ADVISABLE TO USE THE DOOR.\n"
"-1  END\n"
"2\n"
"1    YOU'RE AT END OF ROAD AGAIN.\n"
"2    YOU'RE AT HILL IN ROAD.\n"
"3    YOU'RE INSIDE BUILDING.\n"
"4    YOU'RE IN VALLEY\n"
"5    YOU'RE IN FOREST\n"
"6    YOU'RE IN FOREST\n"
"7    YOU'RE AT SLIT IN STREAMBED\n"
"8    YOU'RE OUTSIDE GRATE\n"
"9    YOU'RE BELOW THE GRATE\n"
"10   YOU'RE IN COBBLE CRAWL\n"
"11   YOU'RE IN DEBRIS ROOM.\n"
"13   YOU'RE IN BIRD CHAMBER.\n"
"14   YOU'RE AT TOP OF SMALL PIT.\n"
"15   YOU'RE IN HALL OF MISTS.\n"
"17   YOU'RE ON EAST BANK OF FISSURE.\n"
"18   YOU'RE IN NUGGET OF GOLD ROOM.\n"
"19   YOU'RE IN HALL OF MT KING.\n"
"33   YOU'RE AT Y2\n"
"35   YOU'RE AT WINDOW ON PIT\n"
"36   YOU'RE IN DIRTY PASSAGE\n"
"39   YOU'RE N DUSTY ROCK ROOM.\n"
"41   YOU'RE AT WEST END OF HALL OF MISTS.\n"
"57   YOU'RE AT BRINK OF PIT.\n"
"60   YOU'RE AT EAST END OF LONG HALL.\n"
"66   YOU'RE IN SWISS CHEESE ROOM\n"
"67   YOU'RE IN TWOPIT ROOM\n"
"68   YOU'RE IN SLAB ROOM\n"
"-1\n"
"3\n"
"1   2   2   44\n"
"1   3   3   12  19  43\n"
"1   4   4   5   13  14  46  30\n"
"1   5   6   45  43\n"
"1   8   49\n"
"2   1   8   2   12  7   43  45  30\n"
"2   5   6   45  46\n"
"3   1   3   11  32  44\n"
"3   11  48\n"
"3   33  65\n"
"3   79  5   14\n"
"4   1   4   45\n"
"4   5   6   43  44  29\n"
"4   7   5   46  30\n"
"4   8   49\n"
"5   4   9   43  30\n"
"5   300 6   7   8   45\n"
"5   5   44  46\n"
"6   1   2   45\n"
"6   4   9   43  44  30\n"
"6   5   6   46\n"
"7   1   12\n"
"7   4   4   45\n"
"7   5   6   43  44\n"
"7   8   5   15  16  46  30\n"
"7   24  47  14  30\n"
"8   5   6   43  44  46\n"
"8   1   12\n"
"8   7   4   13  45\n"
"8   301 3   5   19  30\n"
"9   302 11  12\n"
"9   10  17  18  19  44\n"
"9   14  31\n"
"9   11  51\n"
"10  9   11  20  21  43\n"
"10  11  19  22  44  51\n"
"10  14  31\n"
"11  310 49\n"
"11  10  17  18  23  24  43\n"
"11  12  25  305 19  29  44\n"
"11  3   48\n"
"11  14  31\n"
"12  310 49\n"
"12  11  30  43  51\n"
"12  13  19  29  44\n"
"12  14  31\n"
"13  310 49\n"
"13  11  51\n"
"13  12  25  305 43\n"
"13  14  23  31  44\n"
"14  310 49\n"
"14  11  51\n"
"14  13  23  43\n"
"14  303 30  31  34\n"
"14  16  33  44\n"
"15  18  36  46\n"
"15  17  7   38  44\n"
"15  19  10  30  45\n"
"15  304 29  31  34  35  23  43\n"
"15  34  55\n"
"15  62  69\n"
"16  14  1\n"
"17  15  8   38  43\n"
"17  305 7\n"
"17  306 40  41  42  44  19  39\n"
"18  15  38  11  8   45\n"
"19  15  10  29  43\n"
"19  307 45  36\n"
"19  308 46  37\n"
"19  309 44  7\n"
"19  74  66\n"
"20  26  1\n"
"21  26  1\n"
"22  15  1\n"
"23  8   1\n"
"24  7   1\n"
"25  9   1\n"
"27  17  8   11  38\n"
"27  40  45\n"
"27  41  44\n"
"28  19  38  11  46\n"
"28  33  45\n"
"28  36  30  52\n"
"29  19  38  11  45\n"
"30  19  38  11  43\n"
"30  62  44  29\n"
"31  17  1\n"
"32  19  1\n"
"33  3   65\n"
"33  28  46\n"
"33  34  43  53  54\n"
"33  35  44\n"
"34  33  30\n"
"34  15  29\n"
"35  33  43  55\n"
"36  37  43  17\n"
"36  28  29  52\n"
"36  39  44\n"
"37  36  44  17\n"
"37  38  30  31  56\n"
"38  37  56  29\n"
"39  36  43\n"
"39  64  30  52  58\n"
"39  65  70\n"
"40  41  1\n"
"41  42  46  29  23  56\n"
"41  27  43\n"
"41  59  45\n"
"41  60  44  17\n"
"42  41  44\n"
"42  43  43\n"
"42  44  46\n"
"43  42  44\n"
"43  44  46\n"
"43  45  43\n"
"44  42  45\n"
"44  43  43\n"
"44  48  30\n"
"44  50  46\n"
"45  43  45\n"
"45  46  43\n"
"45  47  46\n"
"46  45  44  11\n"
"47  45  45  11\n"
"48  44  29  11\n"
"49  50  30  43\n"
"49  51  44\n"
"50  44  43\n"
"50  49  44  29\n"
"50  52  46\n"
"51  49  44\n"
"51  52  43\n"
"51  53  46\n"
"52  50  45\n"
"52  51  44\n"
"52  53  29\n"
"52  55  43\n"
"53  51  44\n"
"53  52  45\n"
"53  54  46\n"
"54  53  43  11\n"
"55  52  44\n"
"55  56  30\n"
"55  57  43\n"
"56  55  29  11\n"
"57  55  44\n"
"57  58  46\n"
"57  13  30  56\n"
"58  57  44  11\n"
"59  27  1\n"
"60  41  43  29\n"
"60  61  44\n"
"60  62  45  30\n"
"61  60  43  11\n"
"62  60  44\n"
"62  63  45\n"
"62  30  43\n"
"62  15  46\n"
"63  62  46  11\n"
"64  39  29  56  59\n"
"64  65  44\n"
"65  64  43\n"
"65  66  44\n"
"65  68  61\n"
"65  311 46\n"
"65  312 29\n"
"66  313 45\n"
"66  65  60\n"
"66  67  44\n"
"66  77  25\n"
"66  314 46\n"
"67  66  43\n"
"67  72  60\n"
"68  66  46\n"
"68  69  29\n"
"69  68  30\n"
"69  74  46\n"
"70  71  45\n"
"71  39  29\n"
"71  65  62\n"
"71  70  46\n"
"72  67  63\n"
"72  73  45\n"
"73  72  46\n"
"74  19  43\n"
"74  69  44\n"
"74  75  30\n"
"75  76  46\n"
"75  77  45\n"
"76  75  45\n"
"77  75  43\n"
"77  78  44\n"
"77  66  45\n"
"78  77  46\n"
"79  3   1\n"
"-1\n"
"4\n"
"2   ROAD\n"
"3   ENTER\n"
"3   DOOR\n"
"3   GATE\n"
"4   UPSTR\n"
"5   DOWNS\n"
"6   FORES\n"
"7   FORWA\n"
"7   CONTI\n"
"7   ONWAR\n"
"8   BACK\n"
"8   RETUR\n"
"8   RETRE\n"
"9   VALLE\n"
"10  STAIR\n"
"11  OUT\n"
"11  OUTSI\n"
"11  EXIT\n"
"11  LEAVE\n"
"12  BUILD\n"
"12  BLD\n"
"12  HOUSE\n"
"13  GULLY\n"
"14  STREA\n"
"15  ROCK\n"
"16  BED\n"
"17  CRAWL\n"
"18  COBBL\n"
"19  INWAR\n"
"19  INSID\n"
"19  IN\n"
"20  SURFA\n"
"21  NULL\n"
"21  NOWHE\n"
"22  DARK\n"
"23  PASSA\n"
"24  LOW\n"
"25  CANYO\n"
"26  AWKWA\n"
"29  UPWAR\n"
"29  UP\n"
"29  U\n"
"29  ABOVE\n"
"30  D\n"
"30  DOWNW\n"
"30  DOWN\n"
"31  PIT\n"
"32  OUTDO\n"
"33  CRACK\n"
"34  STEPS\n"
"35  DOME\n"
"36  LEFT\n"
"37  RIGHT\n"
"38  HALL\n"
"39  JUMP\n"
"40  MAGIC\n"
"41  OVER\n"
"42  ACROS\n"
"43  EAST\n"
"43  E\n"
"44  WEST\n"
"44  W\n"
"45  NORTH\n"
"45  N\n"
"46  SOUTH\n"
"46  S\n"
"47  SLIT\n"
"48  XYZZY\n"
"49  DEPRE\n"
"50  ENTRA\n"
"51  DEBRI\n"
"52  HOLE\n"
"53  WALL\n"
"54  BROKE\n"
"55  Y2\n"
"56  CLIMB\n"
"57  LOOK\n"
"57  EXAMI\n"
"57  TOUCH\n"
"57  LOOKA\n"
"58  FLOOR\n"
"59  ROOM\n"
"60  NE\n"
"61  SLAB\n"
"61  SLABR\n"
"62  SE\n"
"63  SW\n"
"64  NW\n"
"65  PLUGH\n"
"66  SECRE\n"
"67  CAVE\n"
"68  TURN\n"
"69  CROSS\n"
"70  BEDQU\n"
"1001    KEYS\n"
"1001    KEY\n"
"1002    LAMP\n"
"1002    HEADL\n"
"1003    GRATE\n"
"1004    CAGE\n"
"1005    ROD\n"
"1006    STEPS\n"
"1007    BIRD\n"
"1010    NUGGE\n"
"1010    GOLD\n"
"1011    SNAKE\n"
"1012    FISSU\n"
"1013    DIAMO\n"
"1014    SILVE\n"
"1014    BARS\n"
"1015    JEWEL\n"
"1016    COINS\n"
"1017    DWARV\n"
"1017    DWARF\n"
"1018    KNIFE\n"
"1018    KNIVE\n"
"1018    ROCK\n"
"1018    WEAPO\n"
"1018    BOULD\n"
"1019    FOOD\n"
"1019    RATIO\n"
"1020    WATER\n"
"1020    BOTTL\n"
"1021    AXE\n"
"1022    KNIFE\n"
"1023    CHEST\n"
"1023    BOX\n"
"1023    TREAS\n"
"2001    TAKE\n"
"2001    CARRY\n"
"2001    KEEP\n"
"2001    PICKU\n"
"2001    PICK\n"
"2001    WEAR\n"
"2001    CATCH\n"
"2001    STEAL\n"
"2001    CAPTU\n"
"2001    FIND\n"
"2001    WHERE\n"
"2001    GET\n"
"2002    RELEA\n"
"2002    FREE\n"
"2002    DISCA\n"
"2002    DROP\n"
"2002    DUMP\n"
"2003    DUMMY\n"
"2004    UNLOC\n"
"2004    OPEN\n"
"2004    LIFT\n"
"2005    NOTHI\n"
"2005    HOLD\n"
"2006    LOCK\n"
"2006    CLOSE\n"
"2007    LIGHT\n"
"2007    ON\n"
"2008    EXTIN\n"
"2008    OFF\n"
"2009    STRIK\n"
"2010    CALM\n"
"2010    WAVE\n"
"2010    SHAKE\n"
"2010    SING\n"
"2010    CLEAV\n"
"2011    WALK\n"
"2011    RUN\n"
"2011    TRAVE\n"
"2011    GO\n"
"2011    PROCE\n"
"2011    CONTI\n"
"2011    EXPLO\n"
"2011    GOTO\n"
"2011    FOLLO\n"
"2012    ATTAC\n"
"2012    KILL\n"
"2012    STAB\n"
"2012    FIGHT\n"
"2012    HIT\n"
"2013    POUR\n"
"2014    EAT\n"
"2015    DRINK\n"
"2016    RUB\n"
"3050    OPENS\n"
"3051    HELP\n"
"3051    ?\n"
"3051    WHAT\n"
"3064    TREE\n"
"3066    DIG\n"
"3066    EXCIV\n"
"3067    BLAST\n"
"3068    LOST\n"
"3069    MIST\n"
"3049    THROW\n"
"3079    FUCK\n"
"-1\n"
"5\n"
"201  THERE ARE SOME KEYS ON THE GROUND HERE.\n"
"202  THERE IS A SHINY BRASS LAMP NEARBY.\n"
"3    THE GRATE IS LOCKED\n"
"103  THE GRATE IS OPEN.\n"
"204  THERE IS A SMALL WICKER CAGE DISCARDED NEARBY.\n"
"205  A THREE FOOT BLACK ROD WITH A RUSTY STAR ON AN END LIES NEARBY\n"
"206  ROUGH STONE STEPS LEAD DOWN THE PIT.\n"
"7    A CHEERFUL LITTLE BIRD IS SITTING HERE SINGING.\n"
"107  THERE IS A LITTLE BIRD IN THE CAGE.\n"
"8    THE GRATE IS LOCKED\n"
"108  THE GRATE IS OPEN.\n"
"209  ROUGH STONE STEPS LEAD UP THE DOME.\n"
"210  THERE IS A LARGE SPARKLING NUGGET OF GOLD HERE!\n"
"11   A HUGE GREEN FIERCE SNAKE BARS THE WAY!\n"
"112  A CRYSTAL BRIDGE NOW SPANS THE FISSURE.\n"
"213  THERE ARE DIAMONDS HERE!\n"
"214  THERE ARE BARS OF SILVER HERE!\n"
"215  THERE IS PRECIOUS JEWELRY HERE!\n"
"216  THERE ARE MANY COINS HERE!\n"
"19   THERE IS FOOD HERE.\n"
"20   THERE IS A BOTTLE OF WATER HERE.\n"
"120  THERE IS AN EMPTY BOTTLE HERE.\n"
"221  THERE IS A LITTLE AXE HERE\n"
"-1\n"
"6\n"
"1    SOMEWHERE NEARBY IS COLOSSAL CAVE, WHERE OTHERS HAVE FOUND\n"
"1    FORTUNES IN TREASURE AND GOLD, THOUGH IT IS RUMORED\n"
"1    THAT SOME WHO ENTER ARE NEVER SEEN AGAIN. MAGIC IS SAID\n"
"1    TO WORK IN THE CAVE.  I WILL BE YOUR EYES AND HANDS. DIRECT\n"
"1    ME WITH COMMANDS OF 1 OR 2 WORDS.\n"
"1    (ERRORS, SUGGESTIONS, COMPLAINTS TO CROWTHER)\n"
"1    (IF STUCK TYPE HELP FOR SOME HINTS)\n"
"2    A LITTLE DWARF WITH A BIG KNIFE BLOCKS YOUR WAY.\n"
"3    A LITTLE DWARF JUST WALKED AROUND A CORNER,SAW YOU, THREW\n"
"3    A LITTLE AXE AT YOU WHICH MISSED, CURSED, AND RAN AWAY.\n"
"4    THERE IS A THREATENING LITTLE DWARF IN THE ROOM WITH YOU!\n"
"5    ONE SHARP NASTY KNIFE IS THROWN AT YOU!\n"
"6    HE GETS YOU!\n"
"7    NONE OF THEM HIT YOU!\n"
"8    A HOLLOW VOICE SAYS 'PLUGH'\n"
"9    THERE IS NO WAY TO GO THAT DIRECTION.\n"
"10   I AM UNSURE HOW YOU ARE FACING. USE COMPASS POINTS OR\n"
"10   NEARBY OBJECTS.\n"
"11   I DON'T KNOW IN FROM OUT HERE. USE COMPASS POINTS OR NAME\n"
"11   SOMETHING IN THE GENERAL DIRECTION YOU WANT TO GO.\n"
"12   I DON'T KNOW HOW TO APPLY THAT WORD HERE.\n"
"13   I DON'T UNDERSTAND THAT!\n"
"14   I ALWAYS UNDERSTAND COMPASS DIRECTIONS, OR YOU CAN NAME\n"
"14   A NEARBY THING TO HEAD THAT WAY.\n"
"15   SORRY, BUT I AM NOT ALLOWED TO GIVE MORE DETAIL. I WILL\n"
"15   REPEAT THE LONG DESCRIPTION OF YOUR LOCATION.\n"
"16   IT IS NOW PITCH BLACK. IF YOU PROCEED YOU WILL LIKELY\n"
"16   FALL INTO A PIT.\n"
"17   IF YOU PREFER, SIMPLY TYPE W RATHER THAN WEST.\n"
"18   ARE YOU TRYING TO CATCH THE BIRD?\n"
"19   THE BIRD IS FRIGHTENED RIGHT NOW AND YOU CANNOT CATCH IT\n"
"19   NO MATTER WHAT YOU TRY. PERHAPS YOU MIGHT TRY LATER.\n"
"20   ARE YOU TRYING TO ATTACK OR AVOID THE SNAKE?\n"
"21   YOU CAN'T KILL THE SNAKE, OR DRIVE IT AWAY, OR AVOID IT,\n"
"21   OR ANYTHING LIKE THAT. THERE IS A WAY TO GET BY, BUT YOU\n"
"21   DON'T HAVE THE NECESSARY RESOURCES RIGHT NOW.\n"
"22   MY WORD FOR HITTING SOMETHING WITH THE ROD IS 'STRIKE'.\n"
"23   YOU FELL INTO A PIT AND BROKE EVERY BONE IN YOUR BODY!\n"
"24   YOU ARE ALREADY CARRYING IT!\n"
"25   YOU CAN'T BE SERIOUS!\n"
"26   THE BIRD WAS UNAFRAID WHEN YOU ENTERED, BUT AS YOU APPROACH\n"
"26   IT BECOMES DISTURBED AND YOU CANNOT CATCH IT.\n"
"27   YOU CAN CATCH THE BIRD, BUT YOU CANNOT CARRY IT.\n"
"28   THERE IS NOTHING HERE WITH A LOCK!\n"
"29   YOU AREN'T CARRYING IT!\n"
"30   THE LITTLE BIRD ATTACKS THE GREEN SNAKE, AND IN AN\n"
"30   ASTOUNDING FLURRY DRIVES THE SNAKE AWAY.\n"
"31   YOU HAVE NO KEYS!\n"
"32   IT HAS NO LOCK.\n"
"33   I DON'T KNOW HOW TO LOCK OR UNLOCK SUCH A THING.\n"
"34   THE GRATE WAS ALREADY LOCKED.\n"
"35   THE GRATE IS NOW LOCKED.\n"
"36   THE GRATE WAS ALREADY UNLOCKED.\n"
"37   THE GRATE IS NOW UNLOCKED.\n"
"38   YOU HAVE NO SOURCE OF LIGHT.\n"
"39   YOUR LAMP IS NOW ON.\n"
"40   YOUR LAMP IS NOW OFF.\n"
"41   STRIKE WHAT?\n"
"42   NOTHING HAPPENS.\n"
"43   WHERE?\n"
"44   THERE IS NOTHING HERE TO ATTACK.\n"
"45   THE LITTLE BIRD IS NOW DEAD. ITS BODY DISAPPEARS.\n"
"46   ATTACKING THE SNAKE BOTH DOESN'T WORK AND IS VERY DANGEROUS.\n"
"47   YOU KILLED A LITTLE DWARF.\n"
"48   YOU ATTACK A LITTLE DWARF, BUT HE DODGES OUT OF THE WAY.\n"
"49   I HAVE TROUBLE WITH THE WORD 'THROW' BECAUSE YOU CAN THROW\n"
"49   A THING OR THROW AT A THING. PLEASE USE DROP OR ATTACK INSTEAD.\n"
"50   GOOD TRY, BUT THAT IS AN OLD WORN-OUT MAGIC WORD.\n"
"51   I KNOW OF PLACES, ACTIONS, AND THINGS. MOST OF MY VOCABULARY\n"
"51   DESCRIBES PLACES AND IS USED TO MOVE YOU THERE. TO MOVE TRY\n"
"51   WORDS LIKE FOREST, BUILDING, DOWNSTREAM, ENTER, EAST, WEST\n"
"51   NORTH, SOUTH, UP, OR DOWN.  I KNOW ABOUT A FEW SPECIAL OBJECTS,\n"
"51   LIKE A BLACK ROD HIDDEN IN THE CAVE. THESE OBJECTS CAN BE\n"
"51   MANIPULATED USING ONE OF THE ACTION WORDS THAT I KNOW. USUALLY \n"
"51   YOU WILL NEED TO GIVE BOTH THE OBJECT AND ACTION WORDS\n"
"51   (IN EITHER ORDER), BUT SOMETIMES I CAN INFER THE OBJECT FROM\n"
"51   THE VERB ALONE. THE OBJECTS HAVE SIDE EFFECTS - FOR\n"
"51   INSTANCE, THE ROD SCARES THE BIRD.\n"
"51   USUALLY PEOPLE HAVING TROUBLE MOVING JUST NEED TO TRY A FEW\n"
"51   MORE WORDS. USUALLY PEOPLE TRYING TO MANIPULATE AN\n"
"51   OBJECT ARE ATTEMPTING SOMETHING BEYOND THEIR (OR MY!)\n"
"51   CAPABILITIES AND SHOULD TRY A COMPLETELY DIFFERENT TACK.\n"
"51   TO SPEED THE GAME YOU CAN SOMETIMES MOVE LONG DISTANCES\n"
"51   WITH A SINGLE WORD. FOR EXAMPLE, 'BUILDING' USUALLY GETS\n"
"51   YOU TO THE BUILDING FROM ANYWHERE ABOVE GROUND EXCEPT WHEN\n"
"51   LOST IN THE FOREST. ALSO, NOTE THAT CAVE PASSAGES TURN A\n"
"51   LOT, AND THAT LEAVING A ROOM TO THE NORTH DOES NOT GUARANTEE\n"
"51   ENTERING THE NEXT FROM THE SOUTH. GOOD LUCK!\n"
"52   IT MISSES!\n"
"53   IT GETS YOU!\n"
"54   OK\n"
"55   YOU CAN'T UNLOCK THE KEYS.\n"
"56   YOU HAVE CRAWLED AROUND IN SOME LITTLE HOLES AND WOUND UP\n"
"56   BACK IN THE MAIN PASSAGE.\n"
"57   I DON'T KNOW WHERE THE CAVE IS, BUT HEREABOUTS NO STREAM\n"
"57   CAN RUN ON THE SURFACE FOR LONG. I WOULD TRY THE STREAM.\n"
"58   I NEED MORE DETAILED INSTRUCTIONS TO DO THAT.\n"
"59   I CAN ONLY TELL YOU WHAT YOU SEE AS YOU MOVE ABOUT\n"
"59   AND MANIPULATE THINGS. I CANNOT TELL YOU WHERE REMOTE THINGS\n"
"59   ARE.\n"
"60   I DON'T KNOW THAT WORD.\n"
"61   WHAT?\n"
"62   ARE YOU TRYING TO GET INTO THE CAVE?\n"
"63   THE GRATE IS VERY SOLID AND HAS A HARDENED STEEL LOCK. YOU\n"
"63   CANNOT ENTER WITHOUT A KEY, AND THERE ARE NO KEYS NEARBY.\n"
"63   I WOULD RECOMMEND LOOKING ELSEWHERE FOR THE KEYS.\n"
"64   THE TREES OF THE FOREST ARE LARGE HARDWOOD OAK AND MAPLE,\n"
"64   WITH AN OCCASIONAL GROVE OF PINE OR SPRUCE. THERE IS QUITE\n"
"64   A BIT OF UNDERGROWTH, LARGELY BIRCH AND ASH SAPLINGS PLUS\n"
"64   NONDESCRITPT BUSHES OF VARIOUS SORTS. THIS TIME OF YEAR\n"
"64   VISIBILITY IS QUITE RESTRICTED BY ALL THE LEAVES, BUT TRAVEL\n"
"64   IS QUITE EASY IF YOU DETOUR AROUND THE SPRUCE AND BERRY BUSHES.\n"
"65   WELCOME TO ADVENTURE!!  WOULD YOU LIKE INSTRUCTIONS?\n"
"66   DIGGING WITHOUT A SHOVEL IS QUITE IMPRACTICAL: EVEN WITH A\n"
"66   SHOVEL PROGRESS IS UNLIKELY.\n"
"67   BLASTING REQUIRES DYNAMITE.\n"
"68   I'M AS CONFUSED AS YOU ARE.\n"
"69   MIST IS A WHITE VAPOR, USUALLY WATER, SEEN FROM TIME TO TIME\n"
"69   IN CAVERNS. IT CAN BE FOUND ANYWHERE BUT IS FREQUENTLY A SIGN\n"
"69   OF A DEEP PIT LEADING DOWN TO WATER.\n"
"70   YOUR FEET ARE NOW WET.\n"
"71   THERE IS NOTHING HERE TO EAT.\n"
"72   EATEN!\n"
"73   THERE IS NO DRINKABLE WATER HERE.\n"
"74   THE BOTTLE OF WATER IS NOW EMPTY.\n"
"75   RUBBING THE ELECTRIC LAMP IS NOT PARTICULARLY REWARDING.\n"
"75   ANYWAY, NOTHING EXCITING HAPPENS.\n"
"76   PECULIAR.  NOTHING UNEXPECTED HAPPENS.\n"
"77   YOUR BOTTLE IS EMPTY AND THE GROUND IS WET.\n"
"78   YOU CAN'T POUR THAT.\n"
"79   WATCH IT!\n"
"80   WHICH WAY?\n"
"-1\n"
"0\n";
/* ------------------------------------------------------------------------- */
/* Simple console I/O abstraction (replacement for scaffolding::advent_io)   */
/* ------------------------------------------------------------------------- */

static void io_type_str(const char *s)
{
    fputs(s, stdout);
}

static void io_type_int(int n)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", n);
    fputs(buf, stdout);
}

static void io_getline(char *buf, size_t bufsize)
{
    if (!fgets(buf, (int)bufsize, stdin)) {
        /* treat EOF as empty line */
        buf[0] = '\0';
        return;
    }
    /* strip trailing newline */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
}

static double io_ran(int dummy)
{
    /* Simple PRNG wrapper using rand()/RAND_MAX */
    (void)dummy;
    return (double)rand() / (double)RAND_MAX;
}

static void io_trace_location(int loc)
{
    /* In C version we don't trace; hook is left for debugging if desired. */
    (void)loc;
}

/* ------------------------------------------------------------------------- */
/* Core helpers that correspond to Crowther::shift, getin, type_20a5, etc.   */
/* ------------------------------------------------------------------------- */

/* Shift 36-bit unsigned integer VAL by DIST bits (positive: left, negative: right). */
static void shift36(uint_least64_t val, int dist, uint_least64_t *res)
{
    if (dist < 0)
        *res = val >> -dist;
    else
        *res = (val << dist) & 0777777777777ULL;
}

/* Output a FORTRAN 4 (5 chars per 36-bit word) line segment. */
static void type_20a5(
    const uint_least64_t line[23],
    uint_least64_t begin,
    uint_least64_t end)
{
    char buf[23 * 5 + 2];
    size_t pos = 0;
    uint_least64_t i;
    for (i = begin; i <= end; ++i) {
        char tmp[6];
        a5_to_string(line[i], tmp);
        memcpy(buf + pos, tmp, 5);
        pos += 5;
    }
    buf[pos++] = '\n';
    buf[pos] = '\0';
    io_type_str(buf);
}

/* ACCEPT 4A5 equivalent: read user line and split into 4 A5 words. */
static void accept_4A5(uint_least64_t a[6])
{
    char line[256];
    size_t len, i, idx;
    char upper[256];

    io_getline(line, sizeof(line));
    strncpy(upper, line, sizeof(upper) - 1);
    upper[sizeof(upper) - 1] = '\0';
    to_upper_inplace(upper);

    a[0] = 9999; /* unused */
    for (i = 1; i <= 4; ++i)
        a[i] = A5_SPACE;
    a[5] = A5_SPACE;

    len = strlen(upper);
    idx = 0;
    for (i = 0; i < 4; ++i) {
        char tmp[6];
        size_t j;
        for (j = 0; j < 5; ++j) {
            if (idx < len)
                tmp[j] = upper[idx++];
            else
                tmp[j] = ' ';
        }
        tmp[5] = '\0';
        a[i + 1] = as_a5(tmp);
    }
}

/* Display the PAUSE text and wait for the user to type G or X. */
static void pause_game(const char *msg)
{
    char input[64];

    io_type_str("PAUSE: ");
    io_type_str(msg);
    io_type_str("\n");

    for (;;) {
        io_type_str(
            "TO RESUME EXECUTION, TYPE: G\n"
            "TO TERMINATE THE PROGRAM, TYPE: X\n");
        io_getline(input, sizeof(input));
        to_upper_inplace(input);
        if (strcmp(input, "G") == 0) {
            io_type_str("EXECUTION RESUMED\n\n");
            break;
        }
        if (strcmp(input, "X") == 0) {
            io_type_str("EXECUTION TERMINATED.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/* GETIN: parse up to two words from user input in A5 format. */
static void getin(
    uint_least64_t *twow,
    uint_least64_t *b,
    uint_least64_t *c,
    uint_least64_t *d)
{
    int s = 0;
    int j, k;
    uint_least64_t xx, yy, mask;
    uint_least64_t a[6];
    uint_least64_t m2[7] = {
        9999ULL,04000000000ULL,020000000ULL,0100000ULL,0400ULL,02ULL,0ULL
    };

    accept_4A5(a);

    *twow = 0;
    s = 0;
    *b = a[1];

    for (j = 1; j <= 4; ++j) {
        for (k = 1; k <= 5; ++k) {
            uint_least64_t mask1 = 0774000000000ULL;
            if (k != 1)
                mask1 = 0177ULL * m2[k];

            if (((a[j] ^ A5_SPACE) & mask1) == 0)
                goto L3;

            if (s == 0)
                continue;

            *twow = 1;

            shift36(a[j], 7 * (k - 1), &xx);
            shift36(a[j + 1], 7 * (k - 6), &yy);
            mask = 0 - m2[6 - k];
            *c = (xx & mask) + (yy & (-2 - mask));
            goto L4;

L3:
            if (s == 1)
                continue;
            s = 1;
            if (j == 1) {
                *b = (*b & (0 - m2[k])) |
                     (A5_SPACE & ((0 - m2[k]) ^ (uint_least64_t)-1));
            }
        }
    }

L4:
    *d = a[2];
}

/* ------------------------------------------------------------------------- */
/* Adventure engine data structures (C equivalents of the C++ arrays)        */
/* ------------------------------------------------------------------------- */

/* Arrays are sized according to original FORTRAN/C++ declarations. */

static int dloc[11], dseen[11], odloc[11];
static int tk[26];
static int ichain[101], ifixed[101], iplace[101], prop[101], rtext_tab[101];
static int btext[201];
static int abb[301], cond[301], default_[301], iobj[301], key[301], ltext[301], stext[301];
static int ktab[1001], travel[1001];
static uint_least64_t lline[1001][23];      /* description text table */
static uint_least64_t atab[1001];           /* keyword table */

/* JSPKT, IPLT, IFIXT, DTRAV initial values taken from cca.cpp */
static int jspkt[101] = {
    9999,24,29,0,31,0,31,38,38,42,42,43,46,77,71,73,75
};

static int iplt[101] = {
    9999,3,3,8,10,11,14,13,9,15,18,19,17,27,28,29,30,0,0,3,3
};

static int ifixt[101] = {
    9999,0,0,1,0,0,1,0,1,1,0,1,1
};

static int dtrav[21] = {
    9999,36,28,19,30,62,60,41,27,17,15,19,28,36,300,300
};

/* ------------------------------------------------------------------------- */
/* Parsing helpers for advdat_77_03_31                                       */
/* ------------------------------------------------------------------------- */

static const char *advdat_p = advdat_77_03_31;

/* Read next line from embedded advdat into buf (without trailing '\n'). */
static int advdat_read_line(char *buf, size_t bufsize)
{
    size_t i = 0;
    if (!advdat_p || *advdat_p == '\0') {
        buf[0] = '\0';
        return 0;
    }
    while (*advdat_p && *advdat_p != '\n' && i + 1 < bufsize) {
        buf[i++] = *advdat_p++;
    }
    if (*advdat_p == '\n')
        advdat_p++;
    buf[i] = '\0';
    return 1;
}

/* Parse first integer from line (like FORTRAN G-format read). */
static int parse_first_int(const char *line, int *value)
{
    char *end = NULL;
    long v;
    while (*line && isspace((unsigned char)*line))
        ++line;
    if (!*line)
        return 0;
    v = strtol(line, &end, 10);
    if (line == end)
        return 0;
    *value = (int)v;
    return 1;
}

/* rdmap: FORMAT(12G) equivalent */
static void rdmap(int *jkind, int *lkind, int tk_local[26])
{
    char line[256];
    int vals[12];
    int count = 0;
    const char *p;

    if (!advdat_read_line(line, sizeof(line))) {
        fprintf(stderr, "rdmap(): unexpected end of advdat\n");
        exit(EXIT_FAILURE);
    }

    p = line;
    while (*p) {
        while (*p && isspace((unsigned char)*p))
            ++p;
        if (!*p) break;
        vals[count++] = (int)strtol(p, (char **)&p, 10);
        if (count >= 12)
            break;
    }

    if (count < 1) {
        fprintf(stderr, "rdmap(): failed to parse jkind\n");
        exit(EXIT_FAILURE);
    }

    *jkind = vals[0];
    *lkind = (count >= 2) ? vals[1] : 0;

    {
        int i;
        for (i = 1; i <= 10; ++i) {
            if (i + 1 < count)
                tk_local[i] = vals[i + 1];
            else
                tk_local[i] = 0;
        }
    }
}

/* rdtext: FORMAT(1G,20A5) equivalent */
static void rdtext(int *j, uint_least64_t t[23])
{
    char line[512];
    char *p;

    if (!advdat_read_line(line, sizeof(line))) {
        fprintf(stderr, "rdtext(): unexpected end of advdat\n");
        exit(EXIT_FAILURE);
    }

    if (!parse_first_int(line, j)) {
        fprintf(stderr, "rdtext(): failed to parse jkind\n");
        exit(EXIT_FAILURE);
    }

    /* move p to after the integer */
    p = line;
    (void)strtol(p, &p, 10);
    while (*p == ' ')
        ++p;

    t[0] = 9999;
    t[1] = 0;
    t[2] = 0;

    {
        int n = 3;
        char buf[6];
        size_t len;
        to_upper_inplace(p);
        len = strlen(p);
        while (n < 23 && len > 0) {
            size_t copy = len >= 5 ? 5 : len;
            memset(buf, ' ', 5);
            memcpy(buf, p, copy);
            buf[5] = '\0';
            t[n++] = as_a5(buf);
            p += copy;
            len -= copy;
        }
        while (n < 23)
            t[n++] = as_a5("");
    }
}

/* rdkey: FORMAT(G,A5) equivalent */
static void rdkey(int *k, uint_least64_t *a)
{
    char line[128];
    char *p;
    char word[6];
    size_t i;

    if (!advdat_read_line(line, sizeof(line))) {
        fprintf(stderr, "rdkey(): unexpected end of advdat\n");
        exit(EXIT_FAILURE);
    }

    if (!parse_first_int(line, k)) {
        fprintf(stderr, "rdkey(): failed to parse k\n");
        exit(EXIT_FAILURE);
    }

    p = line;
    (void)strtol(p, &p, 10);
    while (*p == ' ')
        ++p;

    for (i = 0; i < 5; ++i) {
        if (p[i] && p[i] != '\n')
            word[i] = (char)toupper((unsigned char)p[i]);
        else
            word[i] = ' ';
    }
    word[5] = '\0';
    *a = as_a5(word);
}

/* ------------------------------------------------------------------------- */
/* SPEAK and YES subroutines                                                 */
/* ------------------------------------------------------------------------- */

static void speak(int it)
{
    int kkt;

    kkt = rtext_tab[it];
    if (kkt == 0)
        return;
L999:
    type_20a5(lline[kkt], 3, lline[kkt][2]);
    ++kkt;
    if (lline[kkt - 1][1] != 0)
        goto L999;
    io_type_str("\n");
}

static void yes_sub(int x, int y, int z, int *yea)
{
    uint_least64_t junk, ia1, ib1;
    uint_least64_t twow;

    speak(x);
    getin(&twow, &ia1, &junk, &ib1);

    if (ia1 == as_a5("NO") || ia1 == as_a5("N"))
        goto L1;

    *yea = 1;
    if (y != 0)
        speak(y);
    return;

L1:
    *yea = 0;
    if (z != 0)
        speak(z);
}

/* ------------------------------------------------------------------------- */
/* Core adventure routine (heavily based on Crowther::adventure in cca.cpp). */
/* ------------------------------------------------------------------------- */

static void adventure(void)
{
    int attack, dtot, id, idark, idetal, idwarf,
        ifirst, ikind, iid, il, ilk, ilong, itemp, iwest;
    int j, jkind, jobj, jverb, k, kk, kq, ktem, l, lkind, ll, loc, lold, ltrubl,
        stick, temp, yea, jspk;
    uint_least64_t a, b, twowds, wd2;

    int i;

    /* item index constants (matching cca.cpp) */
    const int keys      = 1;
    const int lamp      = 2;
    const int grate     = 3;
    const int rod       = 5;
    const int bird      = 7;
    const int nugget    = 10;
    const int snake     = 11;
    const int food      = 19;
    const int water     = 20;
    const int axe       = 21;

    /* Initialize tables that are implicitly zeroed in FORTRAN/C++ */
    memset(dloc, 0, sizeof(dloc));
    memset(dseen, 0, sizeof(dseen));
    memset(odloc, 0, sizeof(odloc));
    memset(tk,   0, sizeof(tk));
    memset(ichain, 0, sizeof(ichain));
    memset(ifixed, 0, sizeof(ifixed));
    memset(iplace, 0, sizeof(iplace));
    memset(prop, 0, sizeof(prop));
    memset(rtext_tab, 0, sizeof(rtext_tab));
    memset(btext, 0, sizeof(btext));
    memset(abb, 0, sizeof(abb));
    memset(cond, 0, sizeof(cond));
    memset(default_, 0, sizeof(default_));
    memset(iobj, 0, sizeof(iobj));
    memset(key, 0, sizeof(key));
    memset(ltext, 0, sizeof(ltext));
    memset(stext, 0, sizeof(stext));
    memset(ktab, 0, sizeof(ktab));
    memset(travel, 0, sizeof(travel));
    memset(lline, 0, sizeof(lline));
    memset(atab, 0, sizeof(atab));

    /* --------------------------------------------------------------------- */
    /* READ THE PARAMETERS (sections 1002..1100 in cca.cpp)                  */
    /* --------------------------------------------------------------------- */

    i = 1;

L1002:
    {
        char line[64];
        if (!advdat_read_line(line, sizeof(line))) {
            fprintf(stderr, "L1002: read ikind failed\n");
            exit(EXIT_FAILURE);
        }
        if (!parse_first_int(line, &ikind)) {
            fprintf(stderr, "L1002: parse ikind failed\n");
            exit(EXIT_FAILURE);
        }
    }

    switch (ikind) {
    case 0: goto L1100; /* end of data */
    case 1: goto L1004; /* long descriptions */
    case 2: goto L1004; /* short descriptions */
    case 3: goto L1013; /* map data */
    case 4: goto L1020; /* keywords */
    case 5: goto L1004; /* game state descriptions */
    case 6: goto L1004; /* events */
    default:
        fprintf(stderr, "L1002: unexpected ikind value\n");
        exit(EXIT_FAILURE);
    }

L1004:
    rdtext(&jkind, lline[i]);
    if (jkind == -1)
        goto L1002;
    for (k = 1; k <= 20; ++k) {
        kk = k;
        if (lline[i][21 - k] != A5_SPACE)
            goto L1007;
    }
    fprintf(stderr, "L1004: unexpected blank line\n");
    exit(EXIT_FAILURE);

L1007:
    lline[i][2] = 20 - kk + 1;
    lline[i][1] = 0;
    if (ikind == 6)
        goto L1023;
    if (ikind == 5)
        goto L1011;
    if (ikind == 1)
        goto L1008;
    if (stext[jkind] != 0)
        goto L1009;
    stext[jkind] = i;
    goto L1010;

L1008:
    if (ltext[jkind] != 0)
        goto L1009;
    ltext[jkind] = i;
    goto L1010;

L1009:
    lline[i - 1][1] = (uint_least64_t)i;

L1010:
    ++i;
    if (i != 1000)
        goto L1004;
    pause_game("TOO MANY LINES");

L1011:
    if (jkind < 200)
        goto L1012;
    if (btext[jkind - 100] != 0)
        goto L1009;
    btext[jkind - 100] = i;
    btext[jkind - 200] = i;
    goto L1010;

L1012:
    if (btext[jkind] != 0)
        goto L1009;
    btext[jkind] = i;
    goto L1010;

L1023:
    if (rtext_tab[jkind] != 0)
        goto L1009;
    rtext_tab[jkind] = i;
    goto L1010;

L1013:
    i = 1;

L1014:
    rdmap(&jkind, &lkind, tk);
    if (jkind == -1)
        goto L1002;
    if (key[jkind] != 0)
        goto L1016;
    key[jkind] = i;
    goto L1017;

L1016:
    travel[i - 1] = -travel[i - 1];

L1017:
    for (l = 1; l <= 10; ++l) {
        if (tk[l] == 0)
            goto L1019;
        travel[i] = lkind * 1024 + tk[l];
        ++i;
        if (i == 1000) {
            fprintf(stderr, "L1017: STOP\n");
            exit(EXIT_FAILURE);
        }
    }

L1019:
    travel[i - 1] = -travel[i - 1];
    goto L1014;

L1020:
    {
        int iu;
        for (iu = 1; iu <= 1000; ++iu) {
            rdkey(&ktab[iu], &atab[iu]);
            if (ktab[iu] == -1)
                goto L1002;
        }
    }
    pause_game("TOO MANY WORDS");

L1100:
    for (i = 1; i <= 100; ++i) {
        iplace[i] = iplt[i];
        ifixed[i] = ifixt[i];
    }

    for (i = 1; i <= 10; ++i)
        cond[i] = 1;
    cond[16] = 2;
    cond[20] = 2;
    cond[21] = 2;
    cond[22] = 2;
    cond[23] = 2;
    cond[24] = 2;
    cond[25] = 2;
    cond[26] = 2;
    cond[31] = 2;
    cond[32] = 2;
    cond[79] = 2;

    for (i = 1; i <= 100; ++i) {
        ktem = iplace[i];
        if (ktem == 0)
            continue;
        if (iobj[ktem] != 0)
            goto L1104;
        iobj[ktem] = i;
        continue;

L1104:
        ktem = iobj[ktem];

L1105:
        if (ichain[ktem] != 0)
            goto L1106;
        ichain[ktem] = i;
        continue;

L1106:
        ktem = ichain[ktem];
        goto L1105;
    }

    idwarf = 0;
    ifirst = 1;
    iwest = 0;
    ilong = 1;
    idetal = 0;
    pause_game("INIT DONE");

    /* --------------------------------------------------------------------- */
    /* Main game loop (labels 1.. etc.), ported from Crowther::adventure.    */
    /* --------------------------------------------------------------------- */

    yes_sub(65, 1, 0, &yea);
    l = 1;
    loc = 1;

L2:
    /* trace_location was test-only in C++; omitted here */

    if (l == 26)
        pause_game("GAME OVER");

    for (i = 1; i <= 3; ++i) {
        if (odloc[i] != l || dseen[i] == 0)
            continue;
        l = loc;
        speak(2);
        goto L74;
    }
L74:
    loc = l;

    if (idwarf != 0)
        goto L60;
    if (loc == 15)
        idwarf = 1;
    goto L71;

L60:
    if (idwarf != 1)
        goto L63;
    if (io_ran(60) > 0.05)
        goto L71;
    idwarf = 2;
    for (i = 1; i <= 3; ++i) {
        dloc[i] = 0;
        odloc[i] = 0;
        dseen[i] = 0;
    }
    speak(3);
    ichain[axe] = iobj[loc];
    iobj[loc] = axe;
    iplace[axe] = loc;
    goto L71;

L63:
    ++idwarf;
    attack = 0;
    dtot = 0;
    stick = 0;
    for (i = 1; i <= 3; ++i) {
        if (2 * i + idwarf < 8)
            continue;
        if (2 * i + idwarf > 23 && dseen[i] == 0)
            continue;
        odloc[i] = dloc[i];
        if (dseen[i] != 0 && loc > 14)
            goto L65;
        dloc[i] = dtrav[i * 2 + idwarf - 8];
        dseen[i] = 0;
        if (dloc[i] != loc && odloc[i] != loc)
            continue;
L65:
        dseen[i] = 1;
        dloc[i] = loc;
        ++dtot;
        if (odloc[i] != dloc[i])
            continue;
        ++attack;
        if (io_ran(65) < 0.1)
            ++stick;
    }
    if (dtot == 0)
        goto L71;
    if (dtot == 1)
        goto L75;
    io_type_str("THERE ARE ");
    io_type_int(dtot);
    io_type_str(" THREATENING LITTLE DWARVES IN THE ROOM WITH YOU.\n");
    goto L77;

L75:
    speak(4);
L77:
    if (attack == 0)
        goto L71;
    if (attack == 1)
        goto L79;
    io_type_str(" ");
    io_type_int(attack);
    io_type_str(" OF THEM THROW KNIVES AT YOU!\n");
    goto L81;

L79:
    speak(5);
    speak(52 + stick);
    if (stick + 1 == 1)
        goto L71;
    if (stick + 1 == 2)
        goto L83;

L81:
    if (stick == 0)
        goto L69;
    if (stick == 1)
        goto L82;
    io_type_str(" ");
    io_type_int(stick);
    io_type_str(" OF THEM GET YOU.\n");
    goto L83;

L82:
    speak(6);

L83:
    pause_game("GAMES OVER");
    goto L71;

L69:
    speak(7);

L71:
    kk = stext[l];
    if (abb[l] == 0 || kk == 0)
        kk = ltext[l];
    if (kk == 0)
        goto L7;

L4:
    type_20a5(lline[kk], 3, lline[kk][2]);
    ++kk;
    if (lline[kk - 1][1] != 0)
        goto L4;
    io_type_str("\n");

L7:
    if (cond[l] == 2)
        goto L8;
    if (loc == 33 && io_ran(7) < 0.25)
        speak(8);
    j = l;
    goto L2000;

L8:
    kk = key[loc];
    if (kk == 0)
        goto L19;
    if (k == 57)
        goto L32;
    if (k == 67)
        goto L40;
    if (k == 8)
        goto L12;
    lold = l;

L9:
    ll = travel[kk];
    if (ll < 0)
        ll = -ll;
    if (1 == (ll % 1024))
        goto L10;
    if (k == (ll % 1024))
        goto L10;
    if (travel[kk] < 0)
        goto L11;
    ++kk;
    goto L9;

L12:
    temp = lold;
    lold = l;
    l = temp;
    goto L21;

L10:
    l = ll / 1024;
    goto L21;

L11:
    jspk = 12;
    if (k >= 43 && k <= 46)
        jspk = 9;
    if (k == 29 || k == 30)
        jspk = 9;
    if (k == 7 || k == 8 || k == 36 || k == 37 || k == 68)
        jspk = 10;
    if (k == 11 || k == 19)
        jspk = 11;
    if (jverb == 1)
        jspk = 59;
    if (k == 48)
        jspk = 42;
    if (k == 17)
        jspk = 80;
    speak(jspk);
    goto L2;

L19:
    speak(13);
    l = loc;
    if (ifirst == 0)
        speak(14);

L21:
    if (l < 300)
        goto L2;
    il = l - 300 + 1;
    switch (il) {
    case  1: goto L22;
    case  2: goto L23;
    case  3: goto L24;
    case  4: goto L25;
    case  5: goto L26;
    case  6: goto L31;
    case  7: goto L27;
    case  8: goto L28;
    case  9: goto L29;
    case 10: goto L30;
    case 11: goto L33;
    case 12: goto L34;
    case 13: goto L36;
    case 14: goto L37;
    case 15: goto L39;
    default:
        break;
    }
    goto L2;

L22:
    l = 6;
    if (io_ran(22) > 0.5)
        l = 5;
    goto L2;

L23:
    l = 23;
    if (iplace[grate] != 0)
        l = 9;
    goto L2;

L24:
    l = 9;
    if (iplace[grate] != 0)
        l = 8;
    goto L2;

L25:
    l = 20;
    if (iplace[nugget] != -1)
        l = 15;
    goto L2;

L26:
    l = 22;
    if (iplace[nugget] != -1)
        l = 14;
    goto L2;

L27:
    l = 27;
    if (prop[12] == 0)
        l = 31;
    goto L2;

L28:
    l = 28;
    if (prop[snake] == 0)
        l = 32;
    goto L2;

L29:
    l = 29;
    if (prop[snake] == 0)
        l = 32;
    goto L2;

L30:
    l = 30;
    if (prop[snake] == 0)
        l = 32;
    goto L2;

L31:
    pause_game("GAME IS OVER");
    goto L1100;

L32:
    if (idetal < 3)
        speak(15);
    ++idetal;
    l = loc;
    abb[l] = 0;
    goto L2;

L33:
    l = 8;
    if (prop[grate] == 0)
        l = 9;
    goto L2;

L34:
    if (io_ran(34) > 0.2)
        goto L35;
    l = 68;
    goto L2;

L35:
    l = 65;

L38:
    speak(56);
    goto L2;

L36:
    if (io_ran(361) > 0.2)
        goto L35;
    l = 39;
    if (io_ran(362) > 0.5)
        l = 70;
    goto L2;

L37:
    l = 66;
    if (io_ran(371) > 0.4)
        goto L38;
    l = 71;
    if (io_ran(372) > 0.25)
        l = 72;
    goto L2;

L39:
    l = 66;
    if (io_ran(39) > 0.2)
        goto L38;
    l = 77;
    goto L2;

L40:
    if (loc < 8)
        speak(57);
    if (loc >= 8)
        speak(58);
    l = loc;
    goto L2;

L2000:
    ltrubl = 0;
    loc = j;
    abb[j] = (abb[j] + 1) % 5;
    idark = 0;
    if (cond[j] % 2 == 1)
        goto L2003;
    if (iplace[2] != j && iplace[2] != -1)
        goto L2001;
    if (prop[2] == 1)
        goto L2003;

L2001:
    speak(16);
    idark = 1;

L2003:
    i = iobj[j];

L2004:
    if (i == 0)
        goto L2011;
    if ((i == 6 || i == 9) && iplace[10] == -1)
        goto L2008;
    ilk = i;
    if (prop[i] != 0)
        ilk = i + 100;
    kk = btext[ilk];
    if (kk == 0)
        goto L2008;

L2005:
    type_20a5(lline[kk], 3, lline[kk][2]);
    ++kk;
    if (lline[kk - 1][1] != 0)
        goto L2005;
    io_type_str("\n");

L2008:
    i = ichain[i];
    goto L2004;

L2012:
    a = wd2;
    b = A5_SPACE;
    twowds = 0;
    goto L2021;

L2009:
    k = 54;

L2010:
    jspk = k;

L5200:
    speak(jspk);

L2011:
    jverb = 0;
    jobj = 0;
    twowds = 0;

L2020:
    getin(&twowds, &a, &wd2, &b);
    k = 70;
    if (a == as_a5("ENTER") && (wd2 == as_a5("STREA") || wd2 == as_a5("WATER")))
        goto L2010;
    if (a == as_a5("ENTER") && twowds)
        goto L2012;

L2021:
    if (a != as_a5("WEST"))
        goto L2023;
    ++iwest;
    if (iwest != 10)
        goto L2023;
    speak(17);

L2023:
    for (i = 1; i <= 1000; ++i) {
        if (ktab[i] == -1)
            goto L3000;
        if (atab[i] == a)
            goto L2025;
    }
    pause_game("ERROR 6");

L2025:
    k = ktab[i] % 1000;
    kq = ktab[i] / 1000 + 1;
    switch (kq) {
    case 1: goto L5014;
    case 2: goto L5000;
    case 3: goto L2026;
    case 4: goto L2010;
    default:
        pause_game("NO NO");
        break;
    }

L2026:
    jverb = k;
    jspk = jspkt[jverb];
    if (twowds != 0)
        goto L2028;
    if (jobj == 0)
        goto L2036;

L2027:
    switch (jverb) {
    case  1: goto L9000;
    case  2: goto L5066;
    case  3: goto L3000;
    case  4: goto L5031;
    case  5: goto L2009;
    case  6: goto L5031;
    case  7: goto L9404;
    case  8: goto L9406;
    case  9: goto L5081;
    case 10: goto L5200;
    case 11: goto L5200;
    case 12: goto L5300;
    case 13: goto L5506;
    case 14: goto L5502;
    case 15: goto L5504;
    case 16: goto L5505;
    default:
        pause_game("ERROR 5");
        break;
    }

L2028:
    a = wd2;
    b = A5_SPACE;
    twowds = 0;
    goto L2023;

L3000:
    jspk = 60;
    if (io_ran(30001) > 0.8)
        jspk = 61;
    if (io_ran(30002) > 0.8)
        jspk = 13;
    speak(jspk);
    ++ltrubl;
    if (ltrubl != 3)
        goto L2020;
    if (j != 13 || iplace[7] != 13 || iplace[5] != -1)
        goto L2032;
    yes_sub(18, 19, 54, &yea);
    goto L2033;

L2032:
    if (j != 19 || prop[11] != 0 || iplace[7] == -1)
        goto L2034;
    yes_sub(20, 21, 54, &yea);
    goto L2033;

L2034:
    if (j != 8 || prop[grate] != 0)
        goto L2035;
    yes_sub(62, 63, 54, &yea);

L2033:
    if (yea == 0)
        goto L2011;
    goto L2020;

L2035:
    if (iplace[5] != j && iplace[5] != -1)
        goto L2020;
    if (jobj != 5)
        goto L2020;
    speak(22);
    goto L2020;

L2036:
    switch (jverb) {
    case  1: goto L2037;
    case  2: goto L5062;
    case  3: goto L5062;
    case  4: goto L9403;
    case  5: goto L2009;
    case  6: goto L9403;
    case  7: goto L9404;
    case  8: goto L9406;
    case  9: goto L5062;
    case 10: goto L5062;
    case 11: goto L5200;
    case 12: goto L5300;
    case 13: goto L5062;
    case 14: goto L5062;
    case 15: goto L5062;
    case 16: goto L5062;
    default:
        pause_game("OOPS");
        break;
    }

L2037:
    if (iobj[j] == 0 || ichain[iobj[j]] != 0)
        goto L5062;
    for (i = 1; i <= 3; ++i) {
        if (dseen[i] != 0)
            goto L5062;
    }
    jobj = iobj[j];
    goto L2027;

L5062:
    if (b != A5_SPACE)
        goto L5333;
    {
        char sa[6];
        a5_to_string(a, sa);
        io_type_str("  ");
        io_type_str(sa);
        io_type_str(" WHAT?\n");
    }
    goto L2020;

L5333:
    {
        char sa[6], sb[6];
        a5_to_string(a, sa);
        a5_to_string(b, sb);
        io_type_str(" ");
        io_type_str(sa);
        io_type_str(sb);
        io_type_str(" WHAT?\n");
    }
    goto L2020;

L5014:
    if (idark == 0)
        goto L8;
    if (io_ran(5014) > 0.25)
        goto L8;
    speak(23);
    pause_game("GAME IS OVER");
    goto L2011;

L5000:
    jobj = k;
    if (twowds != 0)
        goto L2028;
    if (j == iplace[k] || iplace[k] == -1)
        goto L5004;
    if (k != grate)
        goto L502;
    if (j == 1 || j == 4 || j == 7)
        goto L5098;
    if (j > 9 && j < 15)
        goto L5097;

L502:
    if (b != A5_SPACE)
        goto L5316;
    {
        char sa[6];
        a5_to_string(a, sa);
        io_type_str(" I SEE NO ");
        io_type_str(sa);
        io_type_str(" HERE.\n");
    }
    goto L2011;

L5316:
    {
        char sa[6], sb[6];
        a5_to_string(a, sa);
        a5_to_string(b, sb);
        io_type_str(" I SEE NO ");
        io_type_str(sa);
        io_type_str(sb);
        io_type_str(" HERE.\n");
    }
    goto L2011;

L5098:
    k = 49;
    goto L5014;

L5097:
    k = 50;
    goto L5014;

L5004:
    jobj = k;
    if (jverb != 0)
        goto L2027;
    if (b != A5_SPACE)
        goto L5314;
    {
        char sa[6];
        a5_to_string(a, sa);
        io_type_str(" WHAT DO YOU WANT TO DO WITH THE ");
        io_type_str(sa);
        io_type_str("?\n");
    }
    goto L2020;

L5314:
    {
        char sa[6], sb[6];
        a5_to_string(a, sa);
        a5_to_string(b, sb);
        io_type_str(" WHAT DO YOU WANT TO DO WITH THE ");
        io_type_str(sa);
        io_type_str(sb);
        io_type_str("?\n");
    }
    goto L2020;

L9000:
    if (jobj == 18)
        goto L2009;
    if (iplace[jobj] != j)
        goto L5200;
    if (ifixed[jobj] == 0)
        goto L9002;
    speak(25);
    goto L2011;

L9002:
    if (jobj != bird)
        goto L9004;
    if (iplace[rod] != -1)
        goto L9003;
    speak(26);
    goto L2011;

L9003:
    if (iplace[4] == -1 || iplace[4] == j)
        goto L9004;
    speak(27);
    goto L2011;

L9004:
    iplace[jobj] = -1;

L9005:
    if (iobj[j] != jobj)
        goto L9006;
    iobj[j] = ichain[jobj];
    goto L2009;

L9006:
    itemp = iobj[j];

L9007:
    if (ichain[itemp] == jobj)
        goto L9008;
    itemp = ichain[itemp];
    goto L9007;

L9008:
    ichain[itemp] = ichain[jobj];
    goto L2009;

L9403:
    if (j == 8 || j == 9)
        goto L5105;
    speak(28);
    goto L2011;

L5105:
    jobj = grate;
    goto L2027;

L5066:
    if (jobj == 18)
        goto L2009;
    if (iplace[jobj] != -1)
        goto L5200;
    if (jobj != bird || j != 19 || prop[11] == 1)
        goto L9401;
    speak(30);
    prop[11] = 1;

L5160:
    ichain[jobj] = iobj[j];
    iobj[j] = jobj;
    iplace[jobj] = j;
    goto L2011;

L9401:
    speak(54);
    goto L5160;

L5031:
    if (iplace[keys] != -1 && iplace[keys] != j)
        goto L5200;
    if (jobj != 4)
        goto L5102;
    speak(32);
    goto L2011;

L5102:
    if (jobj != keys)
        goto L5104;
    speak(55);
    goto L2011;

L5104:
    if (jobj == grate)
        goto L5107;
    speak(33);
    goto L2011;

L5107:
    if (jverb == 4)
        goto L5033;
    if (prop[grate] != 0)
        goto L5034;
    speak(34);
    goto L2011;

L5034:
    speak(35);
    prop[grate] = 0;
    prop[8] = 0;
    goto L2011;

L5033:
    if (prop[grate] == 0)
        goto L5109;
    speak(36);
    goto L2011;

L5109:
    speak(37);
    prop[grate] = 1;
    prop[8] = 1;
    goto L2011;

L9404:
    if (iplace[2] != j && iplace[2] != -1)
        goto L5200;
    prop[2] = 1;
    idark = 0;
    speak(39);
    goto L2011;

L9406:
    if (iplace[2] != j && iplace[2] != -1)
        goto L5200;
    prop[2] = 0;
    speak(40);
    goto L2011;

L5081:
    if (jobj != 12)
        goto L5200;
    prop[12] = 1;
    goto L2003;

L5300:
    for (id = 1; id <= 3; ++id) {
        iid = id;
        if (dseen[id] != 0)
            goto L5307;
    }
    if (jobj == 0)
        goto L5062;
    if (jobj == snake)
        goto L5200;
    if (jobj == bird)
        goto L5302;
    speak(44);
    goto L2011;

L5302:
    speak(45);
    iplace[jobj] = 300;
    goto L9005;

L5307:
    if (io_ran(5307) > 0.4)
        goto L5309;
    dseen[iid] = 0;
    odloc[iid] = 0;
    dloc[iid] = 0;
    speak(47);
    goto L5311;

L5309:
    speak(48);

L5311:
    k = 21;
    goto L5014;

L5502:
    if ((iplace[food] != j && iplace[food] != -1) || prop[food] != 0 || jobj != food)
        goto L5200;
    prop[food] = 1;
    jspk = 72;
    goto L5200;

L5504:
    if ((iplace[water] != j && iplace[water] != -1) || prop[water] != 0 || jobj != water)
        goto L5200;
    prop[water] = 1;
    jspk = 74;
    goto L5200;

L5505:
    if (jobj != lamp)
        jspk = 76;
    goto L5200;

L5506:
    if (jobj != water)
        jspk = 78;
    prop[water] = 1;
    goto L5200;
}

/* ------------------------------------------------------------------------- */
/* main()                                                                    */
/* ------------------------------------------------------------------------- */

int main(void)
{
    io_type_str(
        "-----------------------------------------------------------------\n"
        "     Will Crowther's original 1976 \"Colossal Cave Adventure\"\n"
        "               A faithful reimplementation in C\n"
        "          by Erik Lins, 2025  (CC0 1.0) Public Domain\n"
        "          (based on Anthony Hay's C++ version, 2024)\n"
        "-----------------------------------------------------------------\n"
        "To quit hit Ctrl-C\n\n");

    srand((unsigned)time(NULL));

    adventure();
    return 0;
}


