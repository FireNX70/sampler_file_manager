#ifndef S5XX_FS_DRV_TEST_DATA_INCLUDE_GUARD
#define S5XX_FS_DRV_TEST_DATA_INCLUDE_GUARD

#include <array>

#include "Roland/S5XX/S5XX_FS_types.hpp"
#include "min_vfs/min_vfs_base.hpp"

constexpr u8 EXPECTED_HDD_FIRST_DIR_IDX = 0;
constexpr u8 EXPECTED_CD_FIRST_DIR_IDX = 7;

constexpr char BASE_TEST_FS_FNAME[] = "base_test_fs.iso";
constexpr u8 EXPECTED_FIRST_DIR_IDX = EXPECTED_CD_FIRST_DIR_IDX;

const std::array<min_vfs::dentry_t, 3> EXPECTED_ROOT_DIR =
{
	min_vfs::dentry_t("InstrumentGroup", 1 * S5XX::FS::On_disk_sizes::INSTR_GRP, 0, 0, 0, min_vfs::ftype_t::file),
	{
		/*For some fucking reason, g++ will shit the fucking bed if I try to
		 *initialize all dentries with list initializers; but using the implicit
		 *all-members constructor for the first one, and list initializers for
		 *the others will make it fucking work. FUCK. Keep in mind list
		 *initializers within a list initializer works fine for std::vector. On
		 *top of that, manually defaulting the no-members constructor and
		 *defining the all-members one caused g++ to start complaining about
		 *initializer lists all over the fucking place that work fine if I leave
		 *everything implicit. I wanna fucking punch something.*/
		.fname = "SoundDirectory ",
		.fsize = 9 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::dir
	},
	{
		.fname = "InstrumentGroup",
		.fsize = 1 * S5XX::FS::On_disk_sizes::INSTR_GRP,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	}
};

const std::array<min_vfs::dentry_t, 64> EXPECTED_SOUND_DIR =
{
	min_vfs::dentry_t("PSYCH-OUT   TWILIGHTZONE                        ",
					  1746 * S5XX::FS::BLOCK_SIZE, 0, 0, 0,
				   min_vfs::ftype_t::file),
	{
		.fname = "Jupiter 6     Samples      No. 2     Soundtrack ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "W-30        Leya's Song Adrian Scott Copyright  ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "W-30        \"OFFICE\"          AminB  Copyright  ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "W-30 Piano  1)Swing Cafe2)Vignettes  Copyright  ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "W30 DISK    606 -        DRUMATIX               ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "TR-909                                          ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "AC PIANO    THE GAME    PIANO LIVE  18\\04\\1989  ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "HAMMOND                                         ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Drivin'     Dont Take daCar,you'll  KILL Yourslf",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "CHOIR 1                                         ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "CLASSICAL   COMPOSER    SERIES                  ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Club E.PianoSplits W\\Bs & WaveSynth #1          ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "BEST OF D50 MARC GRAUSS  SOUNDS &    SOFTWARE   ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Jungle",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "DISQUETTE-01            DX7==>S550              ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "DISQUETTE 02             DX7==>S550             ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "HARMONICS   RSB #020                            ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "E.GUITAR 1    ELECTRIC    GUITARS               ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "CLEAN STRAT RSB #019                            ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "W-30 UMB'RTOFactory     Patch\\Tone  &E.P.&Electr",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "ELECTRIC    PIANO 2                             ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "E.Organ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "ELECTRIC",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "E.P. & Vibs.Electric         Piano   Vibraphone ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "FAIRLIGHT                                       ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Fm Synth                                        ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "FON JOLE TLB013-356038  Geluiden d.d19 JULI 1989",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "JAZZDRUM                                        ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "BEST OF JX-10                                   ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Synthesizer1                                    ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "PARTIE 1    LA DISQUETTEHORS SERIE !SPECIAL     ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "PARTIE 2    LA DISQUETTEHORS SERIE !SPECIAL     ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Comedy PropsDIY Comedy  Kit &Weirdos+ Vinophone ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "MARC GRAUSS               SOUNDS        &       ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Synth    #3 Synthesizers         #3             ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Orchestra   Hits                                ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "ELECTRIC    ORGAN                               ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "HARPSICORD 1                                    ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "AC",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Pop Organ                                       ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "BIG ROCKERS RSB #024                            ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Synth Vol.2 Synthesizer and MIDI'ed Sounds Vol.2",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Tnr,Sp Sax#1Tenor       Soprano       Saxophones",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "USBVol>SSN08-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "USBVol>SSN09-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "USBVol>SSN10-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "USBVl>SSN10D-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "USBVol>SSN10-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Moog Brass 1            MiniMoog    Big Sweeps  ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "USBVl>SSN10E-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		//Even fucking filenames aren't unique
		.fname = "USBVol>SSN10-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "USBVol>SSN13-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Stomper",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "LARGE STRING                                    ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "Super Kit16 16bit Drums Digitally   Equalized   ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "* T.FISCH *      We        design       the     ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "RX8 DRUMS #1                                    ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "DEMO-01\\1991-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "DEMO-02\\1991-Universal   Sound Bank--STUDIO JBF-",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "SYNTH BASS  RSB #003                            ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "JP-8 BRASS                                      ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "BIZARRE STUFF                                   ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	},
	{
		.fname = "OMINOUS                                         ",
		.fsize = 1746 * S5XX::FS::BLOCK_SIZE,
		.ctime = 0,
		.mtime = 0,
		.atime = 0,
		.ftype = min_vfs::ftype_t::file
	}
};
#endif
