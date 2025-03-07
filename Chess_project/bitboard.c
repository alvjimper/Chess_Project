//system headers
#include <stdio.h>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>
#include <nmmintrin.h>



//define bitboards data type (Little Endian)

#define U64 unsigned long long

//enumerate board squares
enum {
	a8, b8, c8, d8, e8, f8, g8, h8,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a1, b1, c1, d1, e1, f1, g1, h1,
};

//colors

enum{white, black};

//rook and bishop
enum { rook, bishop };

const char* square_to_coordinates[] = {
	"a8","b8","c8","d8","e8","f8","g8","h8",
	"a7","b7","c7","d7","e7","f7","g7","h7",
	"a6","b6","c6","d6","e6","f6","g6","h6",
	"a5","b5","c5","d5","e5","f5","g5","h5",
	"a4","b4","c4","d4","e4","f4","g4","h4",
	"a3","b3","c3","d3","e3","f3","g3","h3",
	"a2","b2","c2","d2","e2","f2","g2","h2",
	"a1","b1","c1","d1","e1","f1","g1","h1",
};
/********************/
/*  Random numbers  */
/********************/

//pseudo random number generator state
unsigned int random_state = 1804289383;

//pseudo random 32-bit number generator
unsigned int get_random_U32()
{
	//initialize random number
	unsigned int number = random_state;
	//XOR shift
	number ^= number << 13;
	number ^= number >> 17;
	number ^= number << 5;

	//set new state
	random_state = number;

	//return random number
	return number;


}
//pseudo random U64 number
U64 get_random_U64()
{
	//initialize 4 random numbers
	U64 n1, n2, n3, n4;
	//initialize random number slicing 16 bits from MS1B side
	n1 = (U64) (get_random_U32()) & 0xFFFF;
	n2 = (U64) (get_random_U32()) & 0xFFFF;
	n3 = (U64) (get_random_U32()) & 0xFFFF;
	n4 = (U64) (get_random_U32()) & 0xFFFF;

	//return random number
	return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}
//generate magic number candidate
U64 generate_magic_candidate()
{
	return get_random_U64() & get_random_U64() & get_random_U64();
}


/********************/
/* Bit Manipulation */
/********************/


//Macros --> set/get/pop

#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define pop_bit(bitboard, square) (bitboard &= ~(1ULL << square))

//count number of bits in a bitboard
#define bit_count(bitboard) _mm_popcnt_u64(bitboard)

/*static inline int bit_count(U64 bitboard)
//{
//	//counter
//	int count = 0;
//	//loop over set bits
//	while (bitboard)
//	{
//		//increment count
//		count++;
//		//reset LS1B
//		bitboard &= bitboard - 1;
//	}
//	return count;
}*/


//get LS1B Index
//#define LS1B_index(bitboard) __builtin_ctzll(bitboard) ¡¡compilador gcc
static inline int get_lsb_index(U64 bitboard) {
	// If bitboard is 0 return -1(illegal index)
	if(bitboard) 
	{	//_BitScanForward64(&index,bitboard) only in VSMC
		return bit_count((bitboard & ~bitboard+1) - 1);
	}else
		return -1;
	
	}
	



//print board

void print_bitboard(U64 bitboard)
{
	printf("\n");
	//loop over board ranks	
	for (int rank = 0; rank < 8; rank++)
	{
		//loop over board files
		for (int file = 0; file < 8; file++)
		{
			//convert file and rank into square index
			int square = rank * 8 + file;

			//print ranks
			if (file == 0) printf("% d", 8 - rank);


			//print bit state (1 or 0)
			printf(" % d", get_bit(bitboard, square) ? 1 : 0);

		}
		//print new line after each rank
		printf("\n");
	}
	//print files
	printf("\n    a  b  c  d  e  f  g  h\n\n");

	//print bitboard as unsigned decimal number
	printf("     Bitboard: %llud\n\n", bitboard);
}


/*********/
/*Attacks*/
/*********/

//not_a_file and not_h_file
const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;

//not HG file 
const U64 not_hg_file = 4557430888798830399ULL;
//not AB file
const U64 not_ab_file = 18229723555195321596ULL;
 
//relevant occupancy bitboards

const int bishop_relevant_occupancy[64] = {
	6,5,5,5,5,5,5,6,
	5,5,5,5,5,5,5,5,
	5,5,7,7,7,7,5,5,
	5,5,7,9,9,7,5,5,
	5,5,7,9,9,7,5,5,
	5,5,7,7,7,7,5,5,
	5,5,5,5,5,5,5,5,
	6,5,5,5,5,5,5,6,
};

const int rook_relevant_occupancy[64] = {
	12,11,11,11,11,11,11,12,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	12,11,11,11,11,11,11,12,
};

//rook magic numbers
U64 rook_magic_numbers[64] = {
	0x10c4048800000082ULL,
	0x10800100021400ULL,
	0x1420810000a00000ULL,
	0x4a80040000042ULL,
	0x300918100000800ULL,
	0x5005000a8080403ULL,
	0x20100200700080ULL,
	0x6400411200444402ULL,
	0x1800c0232440040ULL,
	0x3200a40580200001ULL,
	0x8048a000080000ULL,
	0x2022188000040120ULL,
	0x34840818000002ULL,
	0x1400001004150000ULL,
	0x1800101200ULL,
	0x42018002d80300aULL,
	0x4004500a30401002ULL,
	0x102180b0004121ULL,
	0x4001820403000062ULL,
	0x1520040030002800ULL,
	0x100000008b1080ULL,
	0x2080248200080802ULL,
	0x4892000540400048ULL,
	0x2008000004069210ULL,
	0x504010a040410720ULL,
	0xc1000422802480ULL,
	0x8a80090000400402ULL,
	0x80c002000200000ULL,
	0x620020000348940ULL,
	0x8009008000ULL,
	0x8101282402200aULL,
	0x2902001841ac02e0ULL,
	0x4018a4000100200ULL,
	0x231002a00084088ULL,
	0x12040600810c880ULL,
	0x480480004040015ULL,
	0x200005a0c48002ULL,
	0x98c1226f240020ULL,
	0x2000084044001401ULL,
	0xc408004a001040ULL,
	0x54602100408810ULL,
	0x2000819080200081ULL,
	0x400040402c00004ULL,
	0x62040981c0001032ULL,
	0x2100222002400204ULL,
	0x110400004a001911ULL,
	0x8401002041c88ULL,
	0x1901020400400510ULL,
	0x409810042420040ULL,
	0x8302420c01084200ULL,
	0x40604002c0103000ULL,
	0x48a400048c000008ULL,
	0x4c0088200004c241ULL,
	0x200008990040000ULL,
	0x12081800c0104008ULL,
	0x8012481000210800ULL,
	0x24100080e0864004ULL,
	0x4104204a60812001ULL,
	0x4800000220422080ULL,
	0x422008180c1020ULL,
	0x2308102a00414400ULL,
	0x1a02002820002020ULL,
	0xd880020102a80104ULL,
	0x100008008004980ULL


};

//bishop magic numbers
U64 bishop_magic_numbers[64] = {
	0x4029000001000020ULL,
	0x6400000100004ULL,
	0x9480800100080200ULL,
	0x10000802040003c0ULL,
	0x1010430800024000ULL,
	0x8a0080540180040ULL,
	0x44000200a08801ULL,
	0x410041240190800ULL,
	0x248400008050201ULL,
	0x8400000100002ULL,
	0x16403402c2000000ULL,
	0x1041468000001ULL,
	0x20800000004000ULL,
	0x2084100281084ULL,
	0x2018a80200021000ULL,
	0x1804000b00810100ULL,
	0x10a0404800020ULL,
	0xa008140900120000ULL,
	0x800002808000284ULL,
	0x1040100040421001ULL,
	0x822042000ULL,
	0xccc01088b5106ULL,
	0x41001424280c0002ULL,
	0x104800000103cULL,
	0x5200a10004100804ULL,
	0x10c0d01405001000ULL,
	0x4000000018002201ULL,
	0x4000a6000004ULL,
	0xc809020040000ULL,
	0x4820028000000ULL,
	0x88044000040b89ULL,
	0x400808000c411a40ULL,
	0x1424200012880200ULL,
	0x2000001006481100ULL,
	0xc40450010024000ULL,
	0x400a00a030400a8ULL,
	0x40a00018e000006ULL,
	0x4100010080082040ULL,
	0x401200c1002012ULL,
	0xc004140010000800ULL,
	0x2002080440700040ULL,
	0x8000300019200ULL,
	0x801042040301002ULL,
	0x4160c1c040830060ULL,
	0x8020293001200840ULL,
	0x8005f00080000080ULL,
	0x1080418c0012000ULL,
	0x2081102000284000ULL,
	0x2015002405000ULL,
	0x420800000000100ULL,
	0x8210200000020ULL,
	0x200806200a008ULL,
	0x100a000000818090ULL,
	0x8050002148400ULL,
	0x406800800c002000ULL,
	0x148021000e00a0bULL,
	0x1800048020808a20ULL,
	0x40000400400084ULL,
	0x2000094011ULL,
	0x42c0015000000028ULL,
	0x202002000100608ULL,
	0x802490000048000ULL,
	0x8010200101420014ULL,
	0xc00010000006ULL
};


//pawn attacks table [color][square]

U64 pawn_attacks[2][64];

//knights attacks table [square]
U64 knight_attacks[64];

//king attacks table [square]
U64 king_attacks[64];
//bishop attacks table [square]
U64 bishop_attacks[64];
//rook attacks table [square]
U64 rook_attacks[64];





//generate pawn attacks

U64 mask_pawn_attacks(int color, int square)
{
	//initialize attacks bitboard
	U64 attacks = 0ULL;
	//piece bitboard
	U64 bitboard = 0ULL;
	//set bitboard pieces
	set_bit(bitboard, square);
	
	//white pawn
	if (color == white)
	{
		//generate pawn attacks
		if((bitboard >> 7)& not_a_file) attacks |= (bitboard >> 7);
		if((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);

	}
	//black pawn
	else
	{	//generate pawn attacks
		if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
		if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
	}
	return attacks;
}

//generate knight attacks
U64 mask_knight_attacks(int square)
{
	//initialize attacks bitboard
	U64 attacks = 0ULL;
	//piece bitboard
	U64 bitboard = 0ULL;
	//set bitboard pieces
	set_bit(bitboard, square);
	//generate knight attacks
	if((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
	if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
	if ((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
	if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
	if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
	if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
	if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
	if((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6);

	return attacks;
}

//generate king attacks
U64 mask_king_attacks(int square)
{
	//initialize attacks bitboard
	U64 attacks = 0ULL;
	//piece bitboard
	U64 bitboard = 0ULL;
	//set bitboard pieces
	set_bit(bitboard, square);
	//generate knight attacks
	if(bitboard >> 8) attacks |= (bitboard >> 8);
	if(bitboard << 8) attacks |= (bitboard << 8);
	if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
	if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
	if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
	if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
	if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);
	if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);

	return attacks;
}
//generate bishop attacks
U64 mask_bishop_attacks(int square)
{
	//initialize attacks bitboard
	U64 attacks = 0ULL;
	//initialize ranks and files
	int r, f;
	//initialize target ranks and files
	int tr = square / 8;
	int tf = square % 8;
	//mask relevant bishop occupancy 
	for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f)); //south east
	for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f)); //north east
	for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f)); //north west
	for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f)); //south west
	
	//return attack bitboard

	return attacks;
}
//generate rook attacks
U64 mask_rook_attacks(int square)
{
	//initialize attacks bitboard
	U64 attacks = 0ULL;
	//initialize ranks and files
	int r, f;
	//initialize target ranks and files
	int tr = square / 8;
	int tf = square % 8;
	//mask relevant rook occupancy 
	for (r = tr + 1; r <= 6 ; r++) attacks |= (1ULL << (r * 8 + tf )); //south 
	for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf)); //north
	for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr*8 + f)); //east
	for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f)); //west

	//return attack bitboard

	return attacks;
}
//generate bishop attacks irl
U64 bishop_attacks_irl(int square,U64 block)
{
	//initialize attacks bitboard
	U64 attacks = 0ULL;
	//initialize ranks and files
	int r, f;
	//initialize target ranks and files
	int tr = square / 8;
	int tf = square % 8;
	//generate bishop attacks
	for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)//south east 
	{
		attacks |= (1ULL << (r * 8 + f));
		if ((1ULL << (r * 8 + f)) & block) break;
	} 
	for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)//north east
	{
		attacks |= (1ULL << (r * 8 + f));
		if ((1ULL << (r * 8 + f)) & block) break;
	}
	for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)//north west
	{
		attacks |= (1ULL << (r * 8 + f));
		if ((1ULL << (r * 8 + f)) & block) break;
	}
	for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)//south west
	{
		attacks |= (1ULL << (r * 8 + f));
		if ((1ULL << (r * 8 + f)) & block) break;
	}

	//return attack bitboard

	return attacks;
}
//generate rook attacks irl
U64 rook_attacks_irl(int square, U64 block)
{
	//initialize attacks bitboard
	U64 attacks = 0ULL;
	//initialize ranks and files
	int r, f;
	//initialize target ranks and files
	int tr = square / 8;
	int tf = square % 8;
	//generate bishop attacks
	for (r = tr + 1; r <= 7 ; r++)//south 
	{
		attacks |= (1ULL << (r * 8 + tf));
		if ((1ULL << (r * 8 + tf)) & block) break;
	}
	for (r = tr - 1; r >= 0; r--)//north
	{
		attacks |= (1ULL << (r * 8 + tf));
		if ((1ULL << (r * 8 + tf)) & block) break;
	}
	for (f = tf + 1; f <= 7; f++)//east
	{
		attacks |= (1ULL << (tr * 8 + f));
		if ((1ULL << (tr * 8 + f)) & block) break;
	}
	for (f = tf - 1;f >= 0; f--)//west
	{
		attacks |= (1ULL << (tr * 8 + f));
		if ((1ULL << (tr * 8 + f)) & block) break;
	}

	//return attack bitboard

	return attacks;
}





//initialize pieces attacks
void init_attacks()
{
	//loop over board squares
	for (int square = 0; square < 64; square++)
	{
		//initialize pawn attacks	
		pawn_attacks[white][square] = mask_pawn_attacks(white, square);
		pawn_attacks[black][square] = mask_pawn_attacks(black, square);
		//initialize knight attacks
		knight_attacks[square] = mask_knight_attacks(square);
		//initialize king attacks
		king_attacks[square] = mask_king_attacks(square);
		//initialize bishop attacks
		bishop_attacks[square] = mask_bishop_attacks(square);
		//initialize rook attacks
		rook_attacks[square] = mask_rook_attacks(square);
	}
}

//set occupancy bitboard
U64 set_occupancy(int index, int bit_mask, U64 attack_mask)
{
	//initialize occupancy bitboard
	U64 occupancy = 0ULL;
	//loop over bit mask
	for (int count = 0; count < bit_mask; count++)
	{
		//get LS1B index of attack mask
		int square = get_lsb_index(attack_mask);
		//pop LS1B
		pop_bit(attack_mask, square);
		//check occupancy on board
		if (index & (1 << count))
		{
			//set occupancy bit
			set_bit(occupancy, square);
		}
	}
	//return occupancy bitboard
	return occupancy;
}

/********************/
/*       Magics      */
/********************/

//find the real magic number                               use bishop or rook indistinguishably
U64 find_magic_number(int square, int relevant_occupancy, int rook)
{
	//initialize occupancy bitboard(roo max[4096]>bishop)
	U64 occupancies[4096];

	//initialize attacks bitboard
	U64 attacks[4096];
	//initialize used attacks
	U64 used_attacks[4096];

	//initialize attack mask for piece
	U64 attack_mask = rook ? mask_rook_attacks(square) : mask_bishop_attacks(square);

	//initialize occupancy index
	int occupancy_index = 1<<relevant_occupancy;

	//loop over occupancy indexes
	for (int index = 0; index < occupancy_index; index++)
	{
		//set occupancy bitboard
		occupancies[index] = set_occupancy(index, relevant_occupancy, attack_mask);
		//set attacks bitboard
		attacks[index] = rook ? rook_attacks_irl(square, occupancies[index]) : bishop_attacks_irl(square, occupancies[index]);
	}

	//loop over random numbers
	for (int random_count = 0; random_count < 100000000; random_count++)
	{
		//generate random magic number candidate
		U64 magic_number = generate_magic_candidate();

		//avoid innapropiate magic numbers
		if (bit_count((attack_mask*magic_number)& 0xFF00000000000000) < 6) continue;

		//initialize used attacks
		memset(used_attacks, 0ULL, sizeof(used_attacks));

		//initialize index and fail flag
		int index, fail;

		//test magic index for each occupancy
		for (index = 0, fail = 0; !fail && index < occupancy_index; index++)
		{
			//initialize magic index
			int magic_index = (int)((occupancies[index] * magic_number) >> (64 - relevant_occupancy));
			//check if magic index is used
			if (used_attacks[magic_index] == 0ULL)
			{
				//set magic index
				used_attacks[magic_index] = attacks[index];
			}
			else if (used_attacks[magic_index] != attacks[index])
			{
				//set fail flag
				fail = 1;
			}
			//check if magic number works
			if(!fail)
				return magic_number;
		}
		//magic number failed
		printf("Fail\n");
		return 0ULL;
	}


}
//initialize magic numbers
void init_magics()
{
	//loop over board squares
	for (int square = 0; square < 64; square++)
	{
		
		//initialize rook magic numbers

		rook_magic_numbers[square] = find_magic_number(square, rook_relevant_occupancy[square], rook);
	}

	printf("\n\n\n");
	//loop over board squares
	for (int square = 0; square < 64; square++)
	{

		//initialize bishop magic numbers
		bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_occupancy[square], bishop);
	}
	
}


/**************************/
/*       Initialize all   */
/*************************/

//initialize all variables
void init_all()
{
	//initialize attacks
	init_attacks();
	//initialize magic numbers
	//init_magics();
}

/********************/
/*       Main       */
/********************/

int main()
{
	//initialize all
	init_all();
	
	return 0;
}