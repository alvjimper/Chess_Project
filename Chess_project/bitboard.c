//system headers
#include <stdio.h>



//define bitboards data type

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

/*"a8","b8","c8","d8","e8","f8","g8","h8",
"a7","b7","c7","d7","e7","f7","g7","h7",
"a6","b6","c6","d6","e6","f6","g6","h6",
"a5","b5","c5","d5","e5","f5","g5","h5",
"a4","b4","c4","d4","e4","f4","g4","h4",
"a3","b3","c3","d3","e3","f3","g3","h3",
"a2","b2","c2","d2","e2","f2","g2","h2",
"a1","b1","c1","d1","e1","f1","g1","h1",*/

/********************/
/* Bit Manipulation */
/********************/

//Macros --> set/get/pop

#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define pop_bit(bitboard, square) (bitboard &= ~(1ULL << square))


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




//pawn attacks table [color][square]

U64 pawn_attacks[2][64];

//knights attacks table [square]
U64 knight_attacks[64];

//king attacks table [square]
U64 king_attacks[64];

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
	}
}
 

int main()
{

	init_attacks();
	//loop over board squares
	for (int square = 0; square < 64; square++)
	{
		print_bitboard(king_attacks[square]);
	}
	
	





	return 0;
}