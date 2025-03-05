//system headers
#include <stdio.h>
#include <intrin.h>
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
/*       Main       */
/********************/

int main()
{

	init_attacks();

	

	
	return 0;
}