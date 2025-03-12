//system headers
#include <stdio.h>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>
#include <nmmintrin.h>



//define bitboards data type (Little Endian)

#define U64 unsigned long long

//define FEN  diferent positions
#define empty "8/8/8/8/8/8/8/8 w - -"
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQk- e6 0 1"
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9"

//enumerate board squares
enum {
	a8, b8, c8, d8, e8, f8, g8, h8,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a1, b1, c1, d1, e1, f1, g1, h1, no_square
};

//define encoding of pieces. Upper case for white pieces, lower case for black pieces
enum { P, N, B, R, Q, K, p, n, b, r, q, k };

//colors
enum { white, black, both };

//rook and bishop
enum { rook, bishop };

//0001----->white king side castling , 0010----->white queen side castling, 0100----->black king side castling, 1000----->black queen side castling
//1111----->both colors can castle king side and queen side
enum { wk = 1, wq = 2, bk = 4, bq = 8 };


//

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

//ASCII art pieces
char ascii_pieces[12] = "PNBRQKpnbrqk";


//convert ascii piece to encoded piece
int ascii_to_piece[128] = {
	['P'] = P,['N'] = N,['B'] = B,['R'] = R,['Q'] = Q,['K'] = K,
	['p'] = p,['n'] = n,['b'] = b,['r'] = r,['q'] = q,['k'] = k
};


//define different piece bitboards 2x(pawn,rook,knight,bishop,queen,king)

U64 bitboards[12];


//define occupancy bitboards (white,black,both)

U64 occupancy[3];

//define color to move
int color;

//define en passant square
int en_passant = no_square;

//define castling rights
int castling;


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
	//initialize random number slicing 16 bits from MS1B color
	n1 = (U64)(get_random_U32()) & 0xFFFF;
	n2 = (U64)(get_random_U32()) & 0xFFFF;
	n3 = (U64)(get_random_U32()) & 0xFFFF;
	n4 = (U64)(get_random_U32()) & 0xFFFF;

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

#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

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
	if (bitboard)
	{	//_BitScanForward64(&index,bitboard) only in VSMC
		return bit_count((bitboard & ~bitboard + 1) - 1);
	}
	else
		return -1;

}



/********************/
/*   Input/output   */
/********************/

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


//print board function
void print_board()
{
	//loop over board ranks
	for (int rank = 0; rank < 8; rank++)
	{
		//loop over board files
		for (int file = 0; file < 8; file++)
		{
			//convert file and rank into square index
			int square = rank * 8 + file;
			//print ranks
			if (!file) printf(" %d", 8 - rank);
			
			//initialize piece
			int piece = -1;
			//loop over pieces
			for (int bbpiece = P; bbpiece <= k; bbpiece++)
			{
				//check if piece is on square
				if (get_bit(bitboards[bbpiece], square))
				{
					//set piece
					piece = bbpiece;
					
				}
			}
			//print piece
			printf(" %c", (piece==-1)?'.': ascii_pieces[piece]);
		}
		//print new line after each rank
		printf("\n");
	}
	//print files
	printf("\n   a b c d e f g h\n\n");
	//print color to move
	printf("   Color: %s\n", color == white ? "white" : "black");
	//print en passant square
	printf("   En passant: %s\n", en_passant == no_square ? "no square" : square_to_coordinates[en_passant]);
	//print castling rights
	printf("   Castling: %c%c%c%c\n\n", castling & wk ? 'K' : '-', castling & wq ? 'Q' : '-', castling & bk ? 'k' : '-', castling & bq ? 'q' : '-');
	
	//print new line after board
	printf("\n");
}

//parse FEN notation
void parse_fen(char* fen) {
	//reset board and state variables
	memset(bitboards, 0ULL, sizeof(bitboards));
	memset(occupancy, 0ULL, sizeof(occupancy));
	color = 0;
	en_passant = no_square;
	castling = 0;

	//loop over board ranks
	for (int rank = 0; rank < 8; rank++)
	{
		//loop over board files
		for (int file = 0; file < 8; file++)
		{
			//convert file and rank into square index
			int square = rank * 8 + file;
			//match ascii pieces wiith FEN notation
			if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
			{
				//get piece
				int piece = ascii_to_piece[*fen];
				//set bitboard
				set_bit(bitboards[piece], square);

				//move to next character
				*fen++;
			}
			//match digits in FEN notation
			if (*fen >= '1' && *fen <= '8')
			{
				//initialize offset(convert char 0 to int 0)
				int offset = *fen - '0';
				//initialize piece variable
				int piece = -1;
				//loop over pieces
				for (int bbpiece = P; bbpiece <= k; bbpiece++)
				{
					//check if piece is on current square
					if (get_bit(bitboards[bbpiece], square))
						//set piece
						piece = bbpiece;

					
				}
				//check if current square is empty
				if (piece == -1)
				{
					//decrement file
					file--;
				}

				
				
				//skip offset
				file += offset;
				//move to next character
				*fen++;
			}
			//match rank separator
			if (*fen == '/')
			{
				//move to next character
				*fen++;
			}
		}
		
		
	}
	//move to fen color
	*fen++;
	//set color
	color = (*fen == 'w') ? white : black;
	printf("fen: '%s'\n",fen);
	//move to castling rights
	fen+=2;
	//parse castling rights
	while (*fen != ' ')
	{
		//parse castling rights
		switch (*fen)
		{
		case 'K': castling |= wk; break;
		case 'Q': castling |= wq; break;
		case 'k': castling |= bk; break;
		case 'q': castling |= bq; break;
		case '-': break;
		}
		//move to next character
		*fen++;
	}
	
	//move to en passant square
	*fen++;
	//parse en passant square
	if (*fen != '-')
	{
		//parse file and rank
		int file = fen[0] - 'a';
		int rank = 8 - (*(fen + 1) - '0');
		//set en passant square
		en_passant = rank * 8 + file;
	}
	else en_passant = no_square;
	
	//initialize white and black occupancy
	occupancy[white] = bitboards[P] | bitboards[N] | bitboards[B] | bitboards[R] | bitboards[Q] | bitboards[K];
	occupancy[black] = bitboards[p] | bitboards[n] | bitboards[b] | bitboards[r] | bitboards[q] | bitboards[k];
	occupancy[both] = occupancy[white] | occupancy[black];

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
	0x8a80104000800020ULL,
	0x140002000100040ULL,
	0x2801880a0017001ULL,
	0x100081001000420ULL,
	0x200020010080420ULL,
	0x3001c0002010008ULL,
	0x8480008002000100ULL,
	0x2080088004402900ULL,
	0x800098204000ULL,
	0x2024401000200040ULL,
	0x100802000801000ULL,
	0x120800800801000ULL,
	0x208808088000400ULL,
	0x2802200800400ULL,
	0x2200800100020080ULL,
	0x801000060821100ULL,
	0x80044006422000ULL,
	0x100808020004000ULL,
	0x12108a0010204200ULL,
	0x140848010000802ULL,
	0x481828014002800ULL,
	0x8094004002004100ULL,
	0x4010040010010802ULL,
	0x20008806104ULL,
	0x100400080208000ULL,
	0x2040002120081000ULL,
	0x21200680100081ULL,
	0x20100080080080ULL,
	0x2000a00200410ULL,
	0x20080800400ULL,
	0x80088400100102ULL,
	0x80004600042881ULL,
	0x4040008040800020ULL,
	0x440003000200801ULL,
	0x4200011004500ULL,
	0x188020010100100ULL,
	0x14800401802800ULL,
	0x2080040080800200ULL,
	0x124080204001001ULL,
	0x200046502000484ULL,
	0x480400080088020ULL,
	0x1000422010034000ULL,
	0x30200100110040ULL,
	0x100021010009ULL,
	0x2002080100110004ULL,
	0x202008004008002ULL,
	0x20020004010100ULL,
	0x2048440040820001ULL,
	0x101002200408200ULL,
	0x40802000401080ULL,
	0x4008142004410100ULL,
	0x2060820c0120200ULL,
	0x1001004080100ULL,
	0x20c020080040080ULL,
	0x2935610830022400ULL,
	0x44440041009200ULL,
	0x280001040802101ULL,
	0x2100190040002085ULL,
	0x80c0084100102001ULL,
	0x4024081001000421ULL,
	0x20030a0244872ULL,
	0x12001008414402ULL,
	0x2006104900a0804ULL,
	0x1004081002402ULL


};

//bishop magic numbers
U64 bishop_magic_numbers[64] = {
	 0x40040844404084ULL,
	0x2004208a004208ULL,
	0x10190041080202ULL,
	0x108060845042010ULL,
	0x581104180800210ULL,
	0x2112080446200010ULL,
	0x1080820820060210ULL,
	0x3c0808410220200ULL,
	0x4050404440404ULL,
	0x21001420088ULL,
	0x24d0080801082102ULL,
	0x1020a0a020400ULL,
	0x40308200402ULL,
	0x4011002100800ULL,
	0x401484104104005ULL,
	0x801010402020200ULL,
	0x400210c3880100ULL,
	0x404022024108200ULL,
	0x810018200204102ULL,
	0x4002801a02003ULL,
	0x85040820080400ULL,
	0x810102c808880400ULL,
	0xe900410884800ULL,
	0x8002020480840102ULL,
	0x220200865090201ULL,
	0x2010100a02021202ULL,
	0x152048408022401ULL,
	0x20080002081110ULL,
	0x4001001021004000ULL,
	0x800040400a011002ULL,
	0xe4004081011002ULL,
	0x1c004001012080ULL,
	0x8004200962a00220ULL,
	0x8422100208500202ULL,
	0x2000402200300c08ULL,
	0x8646020080080080ULL,
	0x80020a0200100808ULL,
	0x2010004880111000ULL,
	0x623000a080011400ULL,
	0x42008c0340209202ULL,
	0x209188240001000ULL,
	0x400408a884001800ULL,
	0x110400a6080400ULL,
	0x1840060a44020800ULL,
	0x90080104000041ULL,
	0x201011000808101ULL,
	0x1a2208080504f080ULL,
	0x8012020600211212ULL,
	0x500861011240000ULL,
	0x180806108200800ULL,
	0x4000020e01040044ULL,
	0x300000261044000aULL,
	0x802241102020002ULL,
	0x20906061210001ULL,
	0x5a84841004010310ULL,
	0x4010801011c04ULL,
	0xa010109502200ULL,
	0x4a02012000ULL,
	0x500201010098b028ULL,
	0x8040002811040900ULL,
	0x28000010020204ULL,
	0x6000020202d0240ULL,
	0x8918844842082200ULL,
	0x4010011029020020ULL
};


//pawn attacks table [color][square]

U64 pawn_attacks[2][64];

//knights attacks table [square]
U64 knight_attacks[64];

//king attacks table [square]
U64 king_attacks[64];
//bishop attacks table [square][occupancy]
U64 bishop_attacks[64][512];
//rook attacks table [square][occupancy]
U64 rook_attacks[64][4096];

//bishop masks
U64 bishop_masks[64];
//rook masks
U64 rook_masks[64];





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
		if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
		if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);

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
	if ((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
	if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
	if ((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
	if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
	if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
	if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
	if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
	if ((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6);

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
	if (bitboard >> 8) attacks |= (bitboard >> 8);
	if (bitboard << 8) attacks |= (bitboard << 8);
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
	for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf)); //south 
	for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf)); //north
	for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f)); //east
	for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f)); //west

	//return attack bitboard

	return attacks;
}
//generate bishop attacks irl
U64 bishop_attacks_irl(int square, U64 block)
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
	for (r = tr + 1; r <= 7; r++)//south 
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
	for (f = tf - 1; f >= 0; f--)//west
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
		////initialize bishop attacks
		//bishop_attacks[square] = mask_bishop_attacks(square);
		////initialize rook attacks
		//rook_attacks[square] = mask_rook_attacks(square);
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
	int occupancy_index = 1 << relevant_occupancy;

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
		if (bit_count((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;

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
			if (!fail)
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
//initialice pieces attacks table
void init_pieces_attacks(int bishop)
{
	//loop over board squares
	for (int square = 0; square < 64; square++)
	{
		//initialize bishop and rook masks
		bishop_masks[square] = mask_bishop_attacks(square);
		rook_masks[square] = mask_rook_attacks(square);

		//initializwe current mask
		U64 current_mask = bishop ? bishop_masks[square] : rook_masks[square];

		//initialize occupancy index
		int occupancy_index = 1 << bit_count(current_mask);

		//loop over occupancy indexes
		for (int index = 0; index < occupancy_index; index++)
		{
			//bishop
			if (bishop)
			{
				//initialize current occupancy bitboard
				U64 occupancy = set_occupancy(index, bit_count(current_mask), current_mask);
				//initialize magic index
				int magic_index = (int)((occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_occupancy[square]));
				//intialize bishop attacks
				bishop_attacks[square][magic_index] = bishop_attacks_irl(square, occupancy);


			}
			//rook
			else
			{
				//initialize current occupancy bitboard
				U64 occupancy = set_occupancy(index, bit_count(current_mask), current_mask);
				//initialize magic index
				int magic_index = (int)((occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_occupancy[square]));
				//intialize rook attacks
				rook_attacks[square][magic_index] = rook_attacks_irl(square, occupancy);
			}
		}
	}
}

//get bishop attacks in current board occupancy
static inline U64 get_bishop_attacks(int square, U64 occupancy)
{

	occupancy &= bishop_masks[square];
	occupancy *= bishop_magic_numbers[square];
	occupancy >>= 64 - bishop_relevant_occupancy[square];

	//return bishop attacks
	return bishop_attacks[square][occupancy];

}
//get rook attacks in current board occupancy
static inline U64 get_rook_attacks(int square, U64 occupancy)
{

	occupancy &= rook_masks[square];
	occupancy *= rook_magic_numbers[square];
	occupancy >>= 64 - rook_relevant_occupancy[square];

	//return rook attacks
	return rook_attacks[square][occupancy];

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
	init_magics();
	//initialize pieces attacks
	init_pieces_attacks(bishop);
	init_pieces_attacks(rook);
}

/********************/
/*       Main       */
/********************/

int main()
{
	
	//initialize all
	init_all();

	////set pawn bitboards
	//bitboards[P] = 0x00FF000000000000;
	//bitboards[p] = 0x000000000000FF00;

	////set knight bitboards
	//bitboards[N] = 0x4200000000000000;
	//bitboards[n] = 0x0000000000000042;
	////set bishop bitboards
	//bitboards[B] = 0x2400000000000000;
	//bitboards[b] = 0x0000000000000024;
	////set rook bitboards
	//bitboards[R] = 0x8100000000000000;
	//bitboards[r] = 0x0000000000000081;
	////set queen bitboards
	//bitboards[Q] = 0x0800000000000000;
	//bitboards[q] = 0x0000000000000008;
	////set king bitboards
	//bitboards[K] = 0x1000000000000000;
	//bitboards[k] = 0x0000000000000010;

	//parse FEN notation
	parse_fen(tricky_position);
	print_board();
	parse_fen(start_position);
	print_board();
	
	


	//
	return 0;
}