//system headers
#include <stdio.h>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <unistd.h>




//define bitboards data type (Little Endian)

#define U64 unsigned long long

//define FEN  different positions
#define empty "8/8/8/8/8/8/8/8 b - -"
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define tricky_position   "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/2P1P3/RNBQKBNR w KQkq a3 0 1"
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

//0001----->white king color castling , 0010----->white queen color castling, 0100----->black king color castling, 1000----->black queen color castling
//1111----->both colors can castle king color and queen color
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

//promotion pieces
char promoted_pieces[] = { [Q] = 'q',[R] = 'r',[B] = 'b',[N] = 'n',[q] = 'q',[r] = 'r',[b] = 'b',[n] = 'n' };
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

//"unique" hash key

U64 hash_key;




/*********************/
/*  Time Management  */
/*********************/
// exit from engine flag
int quit = 0;

// UCI "movestogo" command moves counter
int movestogo = 30;

// UCI "movetime" command time counter
int movetime = -1;

// UCI "time" command holder (ms)
int time_var = -1;

// UCI "inc" command's time increment holder
int inc = 0;

// UCI "starttime" command time holder
int starttime = 0;

// UCI "stoptime" command time holder
int stoptime = 0;

// variable to flag time control availability
int timeset = 0;

// variable to flag when the time is up
int stopped = 0;




//get time ms

int get_time_ms()
{
	//get time
	return GetTickCount();
}

int input_waiting()
{

	static int init = 0, pipe;
	static HANDLE inh;
	DWORD dw;

	if (!init)
	{
		init = 1;
		inh = GetStdHandle(STD_INPUT_HANDLE);
		pipe = !GetConsoleMode(inh, &dw);
		if (!pipe)
		{
			SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
			FlushConsoleInputBuffer(inh);
		}
	}

	if (pipe)
	{
		if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
		return dw;
	}

	else
	{
		GetNumberOfConsoleInputEvents(inh, &dw);
		return dw <= 1 ? 0 : dw;
	}
}

// read GUI/user input
void read_input()
{
    // bytes to read holder
    int bytes;

    // GUI/user input
    char input[256] = "", *endc;

    // "listen" to STDIN
    if (input_waiting())
    {
        // tell engine to stop calculating
        stopped = 1;

        // loop to read bytes from STDIN
        do
        {
            // read bytes from STDIN
            bytes=read(fileno(stdin), input, 256);
        }

        // until bytes available
        while (bytes < 0);

        // searches for the first occurrence of '\n'
        endc = strchr(input,'\n');

        // if found new line set value at pointer to 0
        if (endc) *endc=0;

        // if input is available
        if (strlen(input) > 0)
        {
            // match UCI "quit" command
            if (!strncmp(input, "quit", 4))
            {
                // tell engine to terminate exacution
                quit = 1;
            }

            // // match UCI "stop" command
            else if (!strncmp(input, "stop", 4))    {
                // tell engine to terminate exacution
                quit = 1;
            }
        }
    }
}

// a bridge function to interact between search and GUI input
static void communicate() {
	// if time is up break here
    if(timeset == 1 && get_time_ms() > stoptime) {
		// tell engine to stop calculating
		stopped = 1;
	}

    // read GUI input
	read_input();
}



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
//static inline int bit_count (bitboard) {
//	return  _mm_popcnt_u64(bitboard);
//}

//#define bit_count(bitboard) _mm_popcnt_u64(bitboard)

static inline int bit_count(U64 bitboard)
{
	//counter
	int count = 0;
	//loop over set bits
	while (bitboard)
	{
		//increment count
		count++;
		//reset LS1B
		bitboard &= bitboard - 1;
	}
	return count;
}


//get LS1B Index
//#define LS1B_index(bitboard) __builtin_ctzll(bitboard) ��compilador gcc
static inline int get_lsb_index(U64 bitboard) {
	// If bitboard is 0 return -1(illegal index)
	if (bitboard)
	{	//_BitScanForward64(&index,bitboard) only in VSMC
		return bit_count((bitboard & (~bitboard + 1)) - 1);
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
			if (!file) printf("% d", 8 - rank);


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

//random piece keys [piece][square]
U64 piece_keys[12][64];

//random en_passant keys [square]

U64 en_passant_keys[64];

//random castling keys 1111=16

U64 castling_keys[16];



//random color key

U64 color_key;

//initialize random keys for hashing
void init_random_keys() {
	//random seed
	random_state = 1804289383;


	//loop over pieces
	for (int piece = P; piece <= k; piece++)
	{
		//loop over squares
		for (int square = 0; square < 64; square++)
		{	//initialize random piece keys
			piece_keys[piece][square]= get_random_U64();

		}
	}

	//loop over squares
	for (int square = 0; square < 64; square++)
	{
		//initialize random en passant keys
		en_passant_keys[square] = get_random_U64();
	}

	//loop over castling keys
	for (int castle = 0; castle < 16; castle++) {
		//initalize random castling keys
		castling_keys[castle] = get_random_U64();
	}
	//initialize random color key
	color_key = get_random_U64();
}

//generate hash key function


U64 generate_hash_key()
{

	//hash_key
	U64 hash_key_var = 0ULL;

	//temporal bitboard copy
	U64 bitboard;
	//pieces hash
	//loop over pieces
	for (int piece = P; piece <= k; piece++) {
		//initialize bitboard copy
		bitboard = bitboards[piece];

		//loop over pieces in bitboard
		while (bitboard) {
			//get square occupied by piece
			int square = get_lsb_index(bitboard);
			//hash pieces
			hash_key_var ^= piece_keys[piece][square];

			//pop lsb
			pop_bit(bitboard, square);
		}

	}
	//en passant hash
	//if en passant
	if (en_passant!= no_square) {
		//hash en passant

		hash_key_var ^= en_passant_keys[en_passant];

	}

	//castling hash
	//if castling

	hash_key_var ^= castling_keys[castling];


	//color hash
	if (color == black) {
		hash_key_var ^= color_key;
	}

	//return generated hash_key
	return hash_key_var;

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
			printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
		}
		//print new line after each rank
		printf("\n");
	}
	//print files
	printf("\n   a b c d e f g h\n\n");
	//print color to move
	printf("   Color: %s\n", color == white ? "white" : "black");
	//print en passant square
	printf("   En passant: %s\n", (en_passant == no_square) ? "no square" : square_to_coordinates[en_passant]);
	//print castling rights
	printf("   Castling: %c%c%c%c\n\n", (castling & wk) ? 'K' : '-', (castling & wq) ? 'Q' : '-', (castling & bk) ? 'k' : '-', (castling & bq) ? 'q' : '-');

	//print hash key
	printf("hash key: %llx\n", hash_key);


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
				fen++;
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
				fen++;
			}
			//match rank separator
			if (*fen == '/')
			{
				//move to next character
				fen++;
			}
		}


	}
	//move to fen color
	fen++;
	//set color
	(*fen == 'w') ? (color = white) : (color = black);

	//move to castling rights
	fen += 2;
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
		fen++;
	}

	//move to en passant square
	fen++;
	//parse en passant square
	if (*fen != '-')
	{
		//parse file and rank
		int file = fen[0] - 'a';
		int rank = 8 - (fen[1] - '0');
		//set en passant square
		en_passant = rank * 8 + file;
	}
	else en_passant = no_square;

	//initialize white and black occupancy
	occupancy[white] = bitboards[P] | bitboards[N] | bitboards[B] | bitboards[R] | bitboards[Q] | bitboards[K];
	occupancy[black] = bitboards[p] | bitboards[n] | bitboards[b] | bitboards[r] | bitboards[q] | bitboards[k];
	occupancy[both] = occupancy[white] | occupancy[black];

	//initialize hash key
	hash_key = generate_hash_key();

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
	6,5,5,5,5,5,5,6
};

const int rook_relevant_occupancy[64] = {
	12,11,11,11,11,11,11,12,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	11,10,10,10,10,10,10,11,
	12,11,11,11,11,11,11,12
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
	for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
	for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
	for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
	for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

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
	for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
	for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
	for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
	for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

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

//find the real magic number                          use bishop or rook indifferently
U64 find_magic_number(int square, int relevant_bits, int bishop)
{
	// init occupancy
	U64 occupancy[4096];

	// init attack tables
	U64 attacks[4096];

	// init used attacks
	U64 used_attacks[4096];

	// init attack mask for a current piece
	U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);

	// init occupancy indicies
	int occupancy_indicies = 1 << relevant_bits;

	// loop over occupancy indicies
	for (int index = 0; index < occupancy_indicies; index++)
	{
		// init occupancy
		occupancy[index] = set_occupancy(index, relevant_bits, attack_mask);

		// init attacks
		attacks[index] = bishop ? bishop_attacks_irl(square, occupancy[index]) :
			rook_attacks_irl(square, occupancy[index]);
	}

	// test magic numbers loop
	for (int random_count = 0; random_count < 100000000; random_count++)
	{
		// generate magic number candidate
		U64 magic_number = generate_magic_candidate();

		// skip inappropriate magic numbers
		if (bit_count((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;

		// init used attacks
		memset(used_attacks, 0ULL, sizeof(used_attacks));

		// init index & fail flag
		int index, fail;

		// test magic index loop
		for (index = 0, fail = 0; !fail && index < occupancy_indicies; index++)
		{
			// init magic index
			int magic_index = (int)((occupancy[index] * magic_number) >> (64 - relevant_bits));

			// if magic index works
			if (used_attacks[magic_index] == 0ULL)
				// init used attacks
				used_attacks[magic_index] = attacks[index];

			// otherwise
			else if (used_attacks[magic_index] != attacks[index])
				// magic index doesn't work
				fail = 1;
		}

		// if magic number works
		if (!fail)
			// return it
			return magic_number;
	}

	// if magic number doesn't work
	printf("  Magic number fails!\n");
	return 0ULL;
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


		//initialize occupancy bit count
		int occupancy_bit_count = bit_count(current_mask);

		//initialize occupancy index
		int occupancy_index = (1 << occupancy_bit_count);

		//loop over occupancy indexes
		for (int index = 0; index < occupancy_index; index++)
		{
			//bishop
			if (bishop)
			{
				//initialize current occupancy bitboard
				U64 occupancy = set_occupancy(index, occupancy_bit_count, current_mask);
				//initialize magic index
				int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_occupancy[square]);
				//intialize bishop attacks
				bishop_attacks[square][magic_index] = bishop_attacks_irl(square, occupancy);


			}
			//rook
			else
			{
				//initialize current occupancy bitboard
				U64 occupancy = set_occupancy(index, occupancy_bit_count, current_mask);
				//initialize magic index
				int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_occupancy[square]);
				//intialize rook attacks
				rook_attacks[square][magic_index] = rook_attacks_irl(square, occupancy);
			}
		}
	}
}

//get bishop attacks in current board occupancy
static inline U64 get_bishop_attacks(int square, U64 occupancy)
{
	// get bishop attacks assuming current board occupancy
	occupancy &= bishop_masks[square];
	occupancy *= bishop_magic_numbers[square];
	occupancy >>= 64 - bishop_relevant_occupancy[square];

	// return bishop attacks
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
static inline U64 get_queen_attacks(int square, U64 occupancy)
{

	//return queen attacks
	return (get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy));
}




/**************************/
/*    Generate Moves     */
/*************************/


//is square attacked by color
static inline int is_attacked(int square, int color)
{
	// attacked by white pawns
	if ((color == white) && (pawn_attacks[black][square] & bitboards[P])) return 1;

	// attacked by black pawns
	if ((color == black) && (pawn_attacks[white][square] & bitboards[p])) return 1;

	// attacked by knights
	if (knight_attacks[square] & ((color == white) ? bitboards[N] : bitboards[n])) return 1;

	// attacked by bishops
	if (get_bishop_attacks(square, occupancy[both]) & ((color == white) ? bitboards[B] : bitboards[b])) return 1;

	// attacked by rooks
	if (get_rook_attacks(square, occupancy[both]) & ((color == white) ? bitboards[R] : bitboards[r])) return 1;

	// attacked by bishops
	if (get_queen_attacks(square, occupancy[both]) & ((color == white) ? bitboards[Q] : bitboards[q])) return 1;

	// attacked by kings
	if (king_attacks[square] & ((color == white) ? bitboards[K] : bitboards[k])) return 1;

	// else return false
	return 0;
}
//print attacked squares
void print_attacked_squares(int color)
{

	// loop over board ranks
	for (int rank = 0; rank < 8; rank++)
	{
		// loop over board files
		for (int file = 0; file < 8; file++)
		{
			// init square
			int square = rank * 8 + file;

			// print ranks
			if (!file)
				printf("  %d ", 8 - rank);

			// check whether current square is attacked or not
			printf(" %d", is_attacked(square, color) ? 1 : 0);
		}

		// print new line every rank
		printf("\n");
	}

	// print files
	printf("\n     a b c d e f g h\n\n");


	//loop over board squares
	for (int square = 0; square < 64; square++)
	{
		//check if square is attacked
		if (is_attacked(square, color))
		{
			//print attacked square
			printf("Square: %s is attacked by %s\n", square_to_coordinates[square], color == white ? "white" : "black");
		}
	}
}

//generate moves






/********************/
/*       Main       */
/********************/



/*		 6 nibbles  = 1 nibble is 4 bits				 to hex= 0x
*
*			binary representation							hex representation
		0000 0000 0000 0000 0011 1111 source square			0x3f
		0000 0000 0000 1111 1100 0000 target square			0xfc0
		0000 0000 1111 0000 0000 0000 piece					0xf000
		0000 1111 0000 0000 0000 0000 promotion piece		0xf0000
		0001 0000 0000 0000 0000 0000 capture flag			0x100000
		0010 0000 0000 0000 0000 0000 double push flag		0x200000
		0100 0000 0000 0000 0000 0000 en passant flag		0x400000
		1000 0000 0000 0000 0000 0000 castling flag			0x800000

*/

//encode move
#define encode_move(source, target, piece, promotion, capture, double_push, en_passant, castling) ((source) | ((target) << 6) | ((piece) << 12) | ((promotion) << 16) | ((capture) << 20) | ((double_push) << 21) | ((en_passant) << 22) | ((castling) << 23))

//decode move
#define get_source_move(move) (move & 0x3f)
#define get_target_move(move) ((move & 0xfc0) >> 6)
#define get_piece_move(move) ((move & 0xf000) >> 12)
#define get_promotion_move(move) ((move & 0xf0000) >> 16)
#define get_capture_move(move) (move & 0x100000)
#define get_double_push_move(move) (move& 0x200000)
#define get_en_passant_move(move) (move & 0x400000)
#define get_castling_move(move) (move & 0x800000)


//moveslist structure
typedef struct
{
	//moves
	int moves[256];

	//count
	int count;

}moves;


//initialize moves list
static inline void add_move(moves* move_list, int move)
{

	//store move and increment move count
	move_list->moves[move_list->count] = move;

	//increment move count
	move_list->count++;

}



//print moves(for UCI)
void print_moves(int move)
{

	//print move
	if (get_promotion_move(move))
	{
		printf("%s%s%c", square_to_coordinates[get_source_move(move)], square_to_coordinates[get_target_move(move)], promoted_pieces[get_promotion_move(move)]);
	}
	else
	{
		printf("%s%s", square_to_coordinates[get_source_move(move)], square_to_coordinates[get_target_move(move)]);

	}
}
//print moves list
void print_moves_list(moves* move_list)
{
	// empy move list
	if (!move_list->count)
	{
		printf("\n     No move in the move list!\n");
		return;
	}
	printf("\n	move  piece  capture  double_push  en_passant  castling \n\n");
	//loop over moves
	for (int move_count = 0; move_count < move_list->count; move_count++)
	{
		//get move
		int move = move_list->moves[move_count];

		//print move
		printf("	%s%s%c   %c         %d         %d         %d         %d\n", square_to_coordinates[get_source_move(move)],
			square_to_coordinates[get_target_move(move)],
			get_promotion_move(move) ? promoted_pieces[get_promotion_move(move)] : ' ',
			ascii_pieces[get_piece_move(move)],
			get_capture_move(move) ? 1 : 0,
			get_double_push_move(move) ? 1 : 0,
			get_en_passant_move(move) ? 1 : 0,
			get_castling_move(move) ? 1 : 0);
	}

	// print total number of moves
	printf("\n\n     Total number of moves: %d\n\n", move_list->count);



}

//preserve board state
#define copy_board()						\
	U64 bitboard_copy[12], occupancy_copy[3];	\
	int color_copy, en_passant_copy, castling_copy;	\
	memcpy(bitboard_copy, bitboards, sizeof(bitboards));	\
	memcpy(occupancy_copy, occupancy, sizeof(occupancy));	\
	color_copy = color, en_passant_copy = en_passant, castling_copy = castling;	\
	U64 hash_key_copy = hash_key;													\


//restore board state
#define restore_board()						\
	memcpy(bitboards, bitboard_copy, sizeof(bitboards));  \
	memcpy(occupancy, occupancy_copy, sizeof(occupancy));  \
	color = color_copy, en_passant = en_passant_copy, castling = castling_copy; \
	hash_key = hash_key_copy;														\


//move type
enum
{
	all_moves,only_capture
};

//castling rights constant

const int castling_rights_constant[64] = {
	 7,15,15,15, 3,15,15,11,
	15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,
	13,15,15,15,12,15,15,14
};



//doing move on board
static inline int make_move(int move, int move_flag)
{	//move is not capture
	if (move_flag == all_moves)
	{
		//preserve board state
		copy_board();
		//parse move
		int source_square = get_source_move(move);
		int target_square = get_target_move(move);
		int piece = get_piece_move(move);
		int promotion = get_promotion_move(move);
		int capture = get_capture_move(move);
		int double_push = get_double_push_move(move);
		int enpassant = get_en_passant_move(move);
		int castle = get_castling_move(move);


		//move piece
		pop_bit(bitboards[piece], source_square);
		set_bit(bitboards[piece], target_square);

		//hash piece(remove piece from source square and move it to target square in hash key)
		hash_key ^= piece_keys[piece][source_square];//remove from source square
		hash_key ^= piece_keys[piece][target_square];//move to target square

		//dealing with caspture moves
		if (capture)
		{
			//get captured piece index range depending on color
			int first_piece, last_piece;
			//white to move
			if (color == white)
			{
				first_piece = p;
				last_piece = k;
			}
			//black to move
			else
			{
				first_piece = P;
				last_piece = K;
			}
			//loop over opponent pieces
			for (int bb_piece = first_piece; bb_piece <= last_piece; bb_piece++)
			{
				//check if piece is on target square
				if (get_bit(bitboards[bb_piece], target_square))
				{
					//remove captured piece from target square
					pop_bit(bitboards[bb_piece], target_square);
					//remove piece from hash key
					hash_key ^= piece_keys[bb_piece][target_square];
					break;
				}
			}

		}

		//dealing with en promotion moves
		if (promotion)
		{
			//remove pawn
			pop_bit(bitboards[(color==white) ? P: p], target_square);

			//remove pawn
			if (color==white) {
				pop_bit(bitboards[P],target_square);
				//remove pawn from hash key
				hash_key ^= piece_keys[P][target_square];

			}else {
				//remove pawn
				pop_bit(bitboards[p],target_square);
				//remove pawn from hash key
				hash_key ^= piece_keys[p][target_square];
			}



			//add promoted piece
			set_bit(bitboards[promotion], target_square);

			//add piece promotion to hash key
			hash_key ^= piece_keys[promotion][target_square];
		}
		//dealing with en_passant moves
		if (enpassant)
		{
			//remove captured pawn
			(color == white) ? pop_bit(bitboards[p], target_square + 8) : pop_bit(bitboards[P], target_square - 8);
			//white to move
			if (color == white) {
				//remove captured pawn
				pop_bit(bitboards[p], target_square + 8);
				//remove pawn from hash
				hash_key ^= piece_keys[p][target_square+8];

			}
			//black to move
			else {
				//remove captured pawn
				pop_bit(bitboards[P], target_square - 8);
				//remove pawn from hash
				hash_key ^= piece_keys[P][target_square-8];
			}
		}

		//hash en passant(remove en passant square from hash key)
		if (en_passant != no_square) {
			hash_key ^= en_passant_keys[en_passant];
		}


		//reset en passant square
		en_passant = no_square;



		//dealing with double pawn push moves
		if (double_push)
		{
			(color == white) ? (en_passant = target_square + 8) : (en_passant = target_square - 8);

			//hash en passant square
			//white to move
			if (color == white) {
				en_passant = target_square + 8;
				//hash en passant
				hash_key ^= en_passant_keys[en_passant];
			}
			//black to move
			else {
				en_passant = target_square - 8;
				//hash en passant
				hash_key ^= en_passant_keys[en_passant];
			}

		}
		//dealing with castling moves
		if(castle)
		{
			switch (target_square)
			{
			case (g1):
				pop_bit(bitboards[R], h1);
				set_bit(bitboards[R], f1);
				//hash castling rook
				hash_key ^= piece_keys[R][h1];
				hash_key ^= piece_keys[R][f1];

				break;
			case (c1):
				pop_bit(bitboards[R], a1);
				set_bit(bitboards[R], d1);
				//hash castling rook
				hash_key ^= piece_keys[R][a1];
				hash_key ^= piece_keys[R][d1];

				break;
			case (g8):
				pop_bit(bitboards[r], h8);
				set_bit(bitboards[r], f8);
				//hash castling rook
				hash_key ^= piece_keys[r][h8];
				hash_key ^= piece_keys[r][f8];
				break;
			case (c8):
				pop_bit(bitboards[r], a8);
				set_bit(bitboards[r], d8);
				//hash castling rook
				hash_key ^= piece_keys[r][a8];
				hash_key ^= piece_keys[r][d8];
				break;
			}


		}
		//hash castling rights(remove castling)
		hash_key ^= castling_keys[castling];


		//reset castling rights
		castling &= castling_rights_constant[source_square];
		castling &= castling_rights_constant[target_square];

		//hash castling rights(reset castling)
		hash_key ^= castling_keys[castling];



		// reset occupancies
		memset(occupancy, 0ULL, 24);


		//updating occupancy
		occupancy[white] = bitboards[P] | bitboards[N] | bitboards[B] | bitboards[R] | bitboards[Q] | bitboards[K];
		occupancy[black] = bitboards[p] | bitboards[n] | bitboards[b] | bitboards[r] | bitboards[q] | bitboards[k];
		occupancy[both] = occupancy[white] | occupancy[black];

		//change color
		color ^= 1;

		//hash color

		hash_key ^= color_key;


		//debug hash key incremental update
		//build hash key for the updated position after move is made
		/*U64 hash_debug = generate_hash_key();

		//if hash key built doesnt match the incrementally updated one

		if (hash_key != hash_debug) {
			printf("\n move:");
			print_moves(move);
			printf("\n");
			print_board();
			printf("\n hash key: %llx\n",hash_debug);
			getchar();
		}*/

		//check if king is in check
		if (is_attacked((color == white) ? get_lsb_index(bitboards[k]) : get_lsb_index(bitboards[K]), color))
		{
			//illegal move, restore board state
			restore_board();
			//dont move
			return 0;
		}
		else
		{
			//legal move
			return 1;
		}

	}
	//move is capture
	else
	{	//make sure is a capture
		if (get_capture_move(move))
		{
			make_move(move, all_moves);
		}
		//move is not capture
		else
		{	//dont move
			return 0;
		}

	}
}

static inline void generate_moves(moves* move_list)
{

	//initialize move count
	move_list->count = 0;
	//initialize source(extracted from bitboard below) and target squares(extracted from attacks below)
	int source_square, target_square;

	//initialize piece bitboard and the attacks
	U64 bitboard, attacks;

	//loop over pieces
	for (int bbpiece = P; bbpiece <= k; bbpiece++)
	{
		//update piece bitboard copy
		bitboard = bitboards[bbpiece];
		//generate white pawn  and white knight castling moves
		if (color == white)
		{
			//take white pawn index
			if (bbpiece == P)
			{
				//loop over white pawns
				while (bitboard)
				{
					//get source square
					source_square = get_lsb_index(bitboard);
					//get target square
					target_square = source_square - 8;

					//generate different pawn moves
					if (!(target_square < a8) && !get_bit(occupancy[both], target_square))
					{
						//pawn promotion
						if (source_square >= a7 && source_square <= h7)
						{
							//generate pawn promotion moves
							add_move(move_list, encode_move(source_square, target_square, bbpiece, Q, 0, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, R, 0, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, B, 0, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, N, 0, 0, 0, 0));
						}
						else
						{
							//generate single pawn push moves

							add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 0, 0, 0, 0));
							//generate double pawn push moves
							if ((source_square >= a2 && source_square <= h2) && (!get_bit(occupancy[both], target_square - 8)))
							{
								add_move(move_list, encode_move(source_square, target_square - 8, bbpiece, 0, 0, 1, 0, 0));
							}
						}

					}
					//initialize pawn attacks
					attacks = pawn_attacks[color][source_square] & occupancy[black];

					//generate pawn captures
					while (attacks)
					{
						target_square = get_lsb_index(attacks);
						if (source_square >= a7 && source_square <= h7)
						{
							//generate pawn promotion moves
							add_move(move_list, encode_move(source_square, target_square, bbpiece, Q, 1, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, R, 1, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, B, 1, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, N, 1, 0, 0, 0));
						}
						else //normal capture
						{
							add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 1, 0, 0, 0));
						}


						//pop lsb from attacks
						pop_bit(attacks, target_square);
					}
					//generate en passant captures
					if (en_passant != no_square)
					{
						//initialize en passant attacks
						U64 en_passant_attacks = pawn_attacks[color][source_square] & (1ULL << en_passant);
						//generate en passant captures
						if (en_passant_attacks)
						{
							int en_passant_target_square = get_lsb_index(en_passant_attacks);
							add_move(move_list, encode_move(source_square, en_passant_target_square, bbpiece, 0, 1, 0, 1, 0));
						}
					}


					//pop lsb from bitboard copy
					pop_bit(bitboard, source_square);
				}
			}
			//castling moves
			if (bbpiece == K)
			{
				//king color castling
				if (castling & wk)
				{
					//check if squares between are empoty and not attacked
					if (!get_bit(occupancy[both], f1) && !get_bit(occupancy[both], g1))
					{	//check if squares are not attacked
						if (!is_attacked(e1, black) && !is_attacked(f1, black))
						{
							add_move(move_list, encode_move(e1, g1, bbpiece, 0, 0, 0, 0, 1));
						}
					}
				}




				//queen color castling
				if (castling & wq)
				{
					//check if squares between are empoty and not attacked
					if (!get_bit(occupancy[both], d1) && !get_bit(occupancy[both], c1) && !get_bit(occupancy[both], b1))
					{
						if (!is_attacked(e1, black) && !is_attacked(d1, black))
						{
							add_move(move_list, encode_move(e1, c1, bbpiece, 0, 0, 0, 0, 1));
						}
					}
				}
			}

		}
		//generate black pawn  and black knight castling moves
		else
		{
			//take black pawn index
			if (bbpiece == p)
			{
				//loop over black pawns
				while (bitboard)
				{
					//get source square
					source_square = get_lsb_index(bitboard);
					//get target square
					target_square = source_square + 8;

					//generate different pawn moves
					if (!(target_square > h1) && !get_bit(occupancy[both], target_square))
					{
						//pawn promotion
						if (source_square >= a2 && source_square <= h2)
						{
							//generate pawn promotion moves
							add_move(move_list, encode_move(source_square, target_square, bbpiece, q, 0, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, r, 0, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, b, 0, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, n, 0, 0, 0, 0));
						}
						else
						{
							//generate single pawn push moves

							add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 0, 0, 0, 0));

							//generate double pawn push moves
							if ((source_square >= a7 && source_square <= h7) && (!get_bit(occupancy[both], target_square + 8)))
							{
								add_move(move_list, encode_move(source_square, target_square + 8, bbpiece, 0, 0, 1, 0, 0));
							}
						}
					}
					//initialize pawn attacks
					attacks = pawn_attacks[color][source_square] & occupancy[white];

					//generate pawn captures
					while (attacks)
					{
						target_square = get_lsb_index(attacks);
						if (source_square >= a2 && source_square <= h2)
						{
							//generate pawn promotion moves
							add_move(move_list, encode_move(source_square, target_square, bbpiece, q, 1, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, r, 1, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, b, 1, 0, 0, 0));
							add_move(move_list, encode_move(source_square, target_square, bbpiece, n, 1, 0, 0, 0));
						}

						else //normal capture
						{
							add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 1, 0, 0, 0));
						}


						//pop lsb from attacks
						pop_bit(attacks, target_square);
					}
					//generate en passant captures
					if (en_passant != no_square)
					{
						//initialize en passant attacks
						U64 en_passant_attacks = pawn_attacks[color][source_square] & (1ULL << en_passant);
						//generate en passant captures
						if (en_passant_attacks)
						{
							int en_passant_target_square = get_lsb_index(en_passant_attacks);
							add_move(move_list, encode_move(source_square,en_passant_target_square, bbpiece, 0, 1, 0, 1, 0));
						}
					}



					//pop lsb from bitboard copy
					pop_bit(bitboard, source_square);

				}


			}
			//castling moves
			if (bbpiece == k)
			{
				//king color castling
				if (castling & bk)
				{
					//check if squares between are empty and not attacked
					if (!get_bit(occupancy[both], f8) && !get_bit(occupancy[both], g8))
					{	//check if squares are not attacked
						if (!is_attacked(e8, white) && !is_attacked(f8, white))
						{
							add_move(move_list, encode_move(e8, g8, bbpiece, 0, 0, 0, 0, 1));
						}
					}
				}




				//queen color castling
				if (castling & bq)
				{
					//check if squares between are empoty and not attacked
					if (!get_bit(occupancy[both], d8) && !get_bit(occupancy[both], c8) && !get_bit(occupancy[both], b8))
					{
						if (!is_attacked(e8, white) && !is_attacked(d8, white))
						{
							add_move(move_list, encode_move(e8, c8, bbpiece, 0, 0, 0, 0, 1));
						}
					}
				}
			}
		}
		//generate knight moves
		if ((color == white) ? bbpiece == N : bbpiece == n)
		{
			//loop over knights
			while (bitboard)
			{
				//get source square
				source_square = get_lsb_index(bitboard);
				//initialize knight attacks
				attacks = knight_attacks[source_square] & ((color == white) ? ~occupancy[white] : ~occupancy[black]);
				//generate knight moves
				while (attacks)
				{
					//get target square
					target_square = get_lsb_index(attacks);
					//generate knight moves


					//quiet move
					if (!get_bit((color == white) ? occupancy[black] : occupancy[white], target_square))
					{
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 0, 0, 0, 0));
					}

					//capture move
					else
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 1, 0, 0, 0));



					//pop lsb from attacks
					pop_bit(attacks, target_square);
				}
				//pop lsb from bitboard copy
				pop_bit(bitboard, source_square);

			}

		}

		//generate bishop moves
		if ((color == white) ? bbpiece == B : bbpiece == b)
		{
			//loop over bishops
			while (bitboard)
			{
				//get source square
				source_square = get_lsb_index(bitboard);
				//initialize bishop attacks
				attacks = get_bishop_attacks(source_square, occupancy[both]) & ((color == white) ? ~occupancy[white] : ~occupancy[black]);
				//generate bishop moves
				while (attacks)
				{
					//get target square
					target_square = get_lsb_index(attacks);
					//generate knight moves


					//quiet move
					if (!get_bit((color == white) ? occupancy[black] : occupancy[white], target_square))
					{
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 0, 0, 0, 0));
					}

					//capture move
					else
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 1, 0, 0, 0));


					//pop lsb from attacks
					pop_bit(attacks, target_square);
				}
				//pop lsb from bitboard copy
				pop_bit(bitboard, source_square);

			}

		}
		//generate rook moves
		if ((color == white) ? bbpiece == R : bbpiece == r)
		{
			//loop over rooks
			while (bitboard)
			{
				//get source square
				source_square = get_lsb_index(bitboard);
				//initialize rook attacks
				attacks = get_rook_attacks(source_square, occupancy[both]) & ((color == white) ? ~occupancy[white] : ~occupancy[black]);

				//generate rook moves
				while (attacks)
				{
					//get target square
					target_square = get_lsb_index(attacks);
					//generate knight moves


					//quiet move
					if (!get_bit((color == white) ? occupancy[black] : occupancy[white], target_square))
					{
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 0, 0, 0, 0));
					}

					//capture move
					else
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 1, 0, 0, 0));


					//pop lsb from attacks
					pop_bit(attacks, target_square);
				}
				//pop lsb from bitboard copy
				pop_bit(bitboard, source_square);

			}

		}
		//generate queen moves
		if ((color == white) ? bbpiece == Q : bbpiece == q)
		{
			//loop over queens
			while (bitboard)
			{
				//get source square
				source_square = get_lsb_index(bitboard);
				//initialize queen attacks
				attacks = get_queen_attacks(source_square, occupancy[both]) & ((color == white) ? ~occupancy[white] : ~occupancy[black]);
				//loop over attacks
				while (attacks)
				{
					//get target square
					target_square = get_lsb_index(attacks);
					//generate queen moves
					//quiet move
					if (!get_bit((color == white) ? occupancy[black] : occupancy[white], target_square))
					{
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 0, 0, 0, 0));
					}

					//capture move
					else
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 1, 0, 0, 0));



					//pop lsb from attacks
					pop_bit(attacks, target_square);
				}
				//pop lsb from bitboard copy
				pop_bit(bitboard, source_square);

			}

		}

		//generate king moves
		if ((color == white) ? bbpiece == K : bbpiece == k)
		{
			//loop over knights
			while (bitboard)
			{
				//get source square
				source_square = get_lsb_index(bitboard);
				//initialize king attacks
				attacks = king_attacks[source_square] & ((color == white) ? ~occupancy[white] : ~occupancy[black]);
				//loop over attacks
				while (attacks)
				{
					//get target square
					target_square = get_lsb_index(attacks);
					//generate knight moves


					//quiet move
					if (!get_bit((color == white) ? occupancy[black] : occupancy[white], target_square))
					{
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 0, 0, 0, 0));
					}
					//capture move
					else
						add_move(move_list, encode_move(source_square, target_square, bbpiece, 0, 1, 0, 0, 0));


					//pop lsb from attacks
					pop_bit(attacks, target_square);
				}
				//pop lsb from bitboard copy
				pop_bit(bitboard, source_square);

			}

		}

	}

}


/**************************/
/*       Perft            */
/**************************/



//tree nodes(number of different positions reached at given depth)
long nodes;

//perft driver

static inline void perft_driver(int depth)
{
	//reccursion condition
	if (depth == 0)
	{
		//increment nodes count
		nodes++;
		return;
	}
	//create move list
	moves move_list[1];

	//generate moves
	generate_moves(move_list);
	//loop over generated moves
	for (int move_count = 0; move_count < move_list->count; move_count++)
	{
		 move_list->moves[move_count];

		//preserve board state
		copy_board();
		//make move

		if (!make_move(move_list->moves[move_count] , all_moves))
		{	//skip to next move
			continue;
		}
		//call recursive function
		perft_driver(depth - 1);


		//restore board state
		restore_board();
		//create hash_debug
		/*U64 hash_debug = generate_hash_key();
		if (hash_key != hash_debug) {

			printf("\n\n Restore board");
			printf("\n move:");
			print_moves(move_list->moves[move_count]);
			printf("\n");
			print_board();
			printf("\n hash key: %llx\n",hash_debug);
			getchar();
		}*/



	}
}

//perft test
void perft_test(int depth)
{
	printf("\n\nPerft test at depth %d\n", depth);
	//create move list
	moves move_list[1];

	//generate moves
	generate_moves(move_list);
	//initialize timer
	int start_time = get_time_ms();



	//loop over generated moves
	for (int move_count = 0; move_count < move_list->count; move_count++)
	{
		move_list->moves[move_count];

		//preserve board state
		copy_board();
		//make move

		if (!make_move(move_list->moves[move_count], all_moves))
		{	//skip to next move
			continue;
		}
		//get accumulated nodes
		long accumulated_nodes = nodes;

		//call recursive function
		perft_driver(depth - 1);

		//get old nodes
		long old_nodes = nodes-accumulated_nodes;

		//restore board state
		restore_board();

		//print moves
		printf("\n\nMove: %s%s%c  nodes:%ld\n", square_to_coordinates[get_source_move(move_list->moves[move_count])], square_to_coordinates[get_target_move(move_list->moves[move_count])],promoted_pieces[get_promotion_move(move_list->moves[move_count])], old_nodes);

	}

	//print results
	printf("\n\n Depth: %ld\n", depth);
	printf(" Total Nodes: %ld\n", nodes);
	printf(" Time: %d ms\n", get_time_ms()-start_time);

}
/*****************************/
/*       Evaluation          */
/*****************************/

//material score(pieces score)
int material_score[12] = {
	//white P,K,B,R,Q,K
	100,300,350,500,1000,10000,
	//black p,k,b,r,q,k
	-100,-300,-350,-500,-1000,-10000,


};
// pawn positional score
const int pawn_score[64] =
{
	  90,  90,  90,  90,  90,  90,  90,  90,
	30,  30,  30,  40,  40,  30,  30,  30,
	20,  20,  20,  30,  30,  30,  20,  20,
	10,  10,  10,  20,  20,  10,  10,  10,
	 5,   5,  10,  20,  20,   5,   5,   5,
	 0,   0,   0,   5,   5,   0,   0,   0,
	 0,   0,   0, -10, -10,   0,   0,   0,
	 0,   0,   0,   0,   0,   0,   0,   0
};

// knight positional score
const int knight_score[64] =
{
	 -15,-10,-5,-5,-5,-5,-10,-15,
	 -10,-20,  0,  0,  0,  0,-20,-10,
	-5,  0, 10, 20, 20, 20,  0,-5,
	-5,  5, 20, 30, 30, 20,  5,-5,
	-5,  0, 20, 30, 30, 20,  0,-5,
	-5,  5, 10, 20, 20, 10,  5,-5,
	-10,-20,  0,  5,  5,  0,-20,-5,
	-15,-10,-5,-5,-5,-5,-10,-15
};

// bishop positional score
const int bishop_score[64] =
{
	 -20,-10,-10,-10,-10,-10,-10,-20,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-10,  0,  5, 10, 10,  5,  0,-10,
	-10,  5,  5, 20, 20,  5,  5,-10,
	-10,  0, 10, 20, 20, 10,  0,-10,
	-10, 10, 10, 10, 10, 10, 10,-10,
	-10,  5,  0,  0,  0,  0,  5,-10,
	-20,-10,-30,-10,-10,-30,-10,-20

};

// rook positional score
const int rook_score[64] =
{
	 50, 50, 50, 50, 50, 50, 50, 50,
	 50, 50, 50, 50, 50, 50, 50, 50,
	-5,  0,  10,  10,  10,  10,  0, -5,
	-5,  0,  10,  20,  20,  10,  0, -5,
	-5,  0,  10,  20,  20,  10,  0, -5,
	-5,  0,  10,  10,  10,  10,  0, -5,
	-5,  0,  10,  10,  0,  10,  0, -5,
	 0,  0,  0,  20, 20,  0,  0,  0

};

// king positional score
const int king_score[64] =
{
	 0,   0,   0,   0,   0,   0,   0,   0,
	 0,   0,   5,   5,   5,   5,   0,   0,
	 0,   5,   5,  10,  10,   5,   5,   0,
	 0,   5,  10,  20,  20,  10,   5,   0,
	 0,   5,  10,  20,  20,  10,   5,   0,
	 0,   0,   5,  10,  10,   5,   0,   0,
	 0,   5,   5,  -5,  -5,   0,   5,   0,
	 0,   0,   5,   0, -15,   0,  10,   0
};

// mirror positional score tables for opposite color
const int mirror_score[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

//evaluate position
static inline int evaluate_position()
{
	//initialize score
	int score = 0;
	//initialize current piece bitboard
	U64 bitboard;
	//initialize piece and square
	int piece, square;

	//loop over pieces
	for (int bbpiece = P; bbpiece <= k; bbpiece++)
	{
		//update piece bitboard copy
		bitboard = bitboards[bbpiece];

		//loop over pieces in bitboard
		while (bitboard)
		{
			//get piece square
			square = get_lsb_index(bitboard);
			//get piece
			piece = bbpiece;

			//get score
			score += material_score[piece];
			//get positional score
			switch (piece)
			{
			case P:
				score += pawn_score[square];
				break;
			case p:
				score -= pawn_score[mirror_score[square]];
				break;
			case N:
				score += knight_score[square];
				break;
			case n:
				score -= knight_score[mirror_score[square]];
				break;
			case B:
				score += bishop_score[square];
				break;
			case b:
				score -= bishop_score[mirror_score[square]];
				break;
			case R:
				score += rook_score[square];
				break;
			case r:
				score -= rook_score[mirror_score[square]];
				break;
			case K:
				score += king_score[square];
				break;
			case k:
				score -= king_score[mirror_score[square]];
				break;

			}



			//pop lsb from bitboard copy
			pop_bit(bitboard, square);
		}

	}
	//return evaluation based on color
		return (color == white) ? score : -score;

}




/*****************************/
/*       Search Engine       */
/*****************************/


// most valuable victim & less valuable attacker

/*

	(Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
		Pawn   105    205    305    405    505    605
	  Knight   104    204    304    404    504    604
	  Bishop   103    203    303    403    503    603
		Rook   102    202    302    402    502    602
	   Queen   101    201    301    401    501    601
		King   100    200    300    400    500    600

*/

// MVV LVA [attacker piece][victim piece]
static int mvv_lva[12][12] = {
	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

//define maximum number of plies
#define MAX_PLY 128

//killers move [id][ply](not capture, is quiet move that improves score)
int killers[2][MAX_PLY];

//book moves [piece] [square]
int book_moves[12][64];


//PV line in tricky_position : e2a6 b4c3 d2c3 e6d5 g2h3

//PV move --> array of principal variation moves

//PV length [ply]
int pv_length[MAX_PLY];

//PV table [ply][ply] --> array of principal variation indexed by ply(distance to root)
int pv_table[MAX_PLY][MAX_PLY];

 //next current PV(following current PV flag) and score PV move

int current_pv, score_pv;





//half move counter (1 turn is chess are 2 half moves(2 plies) 1 each color)
int ply;


/************************************\
 *
 *      Transposition Table
 *
****************************************/

// hash table size
#define hash_size 0x400000

//no hash constant

#define no_hash 100000

// transposition table flags
#define hash_flag_exact 0
#define hash_flag_alpha 1
#define hash_flag_beta 2

// transposition table data structure
typedef struct {
	U64 hash_key;   // chess position identifier
	int depth;      // current search depth
	int flag;       // flag that tells us the type of node (fail-low/fail-high/PV)
	int score;      // score (alpha/beta/PV)
} tt;               // transposition table (hash table)

// define tansposition table instance
tt transposition_table[hash_size];

// empty tt (hash table)
void clear_transposition_table()
{
	// loop over tt elements
	for (int index = 0; index < hash_size; index++)
	{
		// reset tt elements
		transposition_table[index].hash_key = 0;
		transposition_table[index].depth = 0;
		transposition_table[index].flag = 0;
		transposition_table[index].score = 0;
	}
}


//read hash of transposition table
static inline int read_hash_tt(int alpha, int beta, int depth) {

	//initialize tt pointer that store specific hash entry (current board info)
	tt* hash_pointer=&transposition_table[hash_key % hash_size];

	//check we are in the correct position
	if (hash_pointer->hash_key == hash_key) {
		//check correct depth
		if (hash_pointer->depth >= depth) {
			//check the flag

			//exact PV node score
			if (hash_pointer->flag == hash_flag_exact) {
				//return exact PV node score
				return hash_pointer->score;
			}
			//check if fails low
			if ((hash_pointer->flag == hash_flag_alpha ) && (hash_pointer->score <= alpha)) {
				//return alpha
				return alpha;
			}
			//check in fails high
			if ((hash_pointer->flag == hash_flag_beta) &&(hash_pointer->score >= beta)) {
				//return beta
				return beta;
			}

		}

	}

	//if no has entry
	return no_hash;

}

//record hash to transposition table

static inline void record_hash_to_tt(int score,int depth,int hash_flag) {

	//initialize tt pointer that store specific hash entry (current board info)
	tt* hash_pointer=&transposition_table[hash_key % hash_size];
	// record values in tt
	hash_pointer->hash_key=hash_key;
	hash_pointer->score=score;
	hash_pointer->flag=hash_flag;
	hash_pointer->depth=depth;


}


//enable pv score flag function

static inline void enable_pv_score(moves* move_list)
{
	//set current pv flag to 0
	current_pv = 0;
	//loop over moves in move list
	for (int count = 0; count < move_list->count; count++)
	{
		//check we are in PV move
		if (pv_table[0][ply] == move_list->moves[count])
		{
			//set score pv flag to 1
			score_pv = 1;
			//set current pv flag to 1
			current_pv = 1;

		}
	}


}

/*  =======================
		 Move ordering
	=======================

	1. PV move
	2. Captures in MVV/LVA
	3. 1st killer move
	4. 2nd killer move
	5. History moves
	6. Unsorted moves
*/

//score move
static inline int score_move(int move)
{
	//pv score flag is allowed
	if (score_pv)
	{
		//check if move is in PV
		if (move == pv_table[0][ply])
		{
			//disable pv score flag
			score_pv = 0;

			//add score to move to search first
			return 20000;
		}
	}


	//capture move
	if (get_capture_move(move))
	{

		//initialize target piece
		int target_piece = P;

		//get piece depending to color
		int start_piece, end_piece;

		// color to move
		if (color == white)
		{
			start_piece = p;
			end_piece = k;
		}
		else
		{
			start_piece = P;
			end_piece = K;
		}

		// loop over opposite bitboard to the current color to move
		for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
		{
			// if there is a piece on the target square
			if (get_bit(bitboards[bb_piece], get_target_move(move)))
			{
				// remove captures piece from bitboard
				target_piece = bb_piece;
				break;
			}
		}

		// score move by MVV LVA lookup [attacker][victim]
		return mvv_lva[get_piece_move(move)][target_piece]+10000;
	}

	// score quiet move
	else
	{
		//score first killer move
		if (move == killers[0][ply])
		{
			return 9000;
		}
		//score second killer move
		else if (move == killers[1][ply])
		{
			return 8000;
		}
		//score book move
		else
			return book_moves[get_piece_move(move)][get_target_move(move)];

	}

	return 0;
}
//sort moves in decreasing order
static inline void sort_moves(moves* move_list)
{
	//score moves
	int move_scores[256] = {NULL};

	//loop over moves
	for (int count = 0; count < move_list->count; count++)
	{
		//score move
		move_scores[count] = score_move(move_list->moves[count]);
	}
		//loop over current moves
	for (int current_move = 0; current_move < move_list->count; current_move++)
	{
		//loop over next moves
		for (int next_move = current_move + 1; next_move < move_list->count; next_move++)
		{
			//check if move is better
			if (move_scores[current_move] < move_scores[next_move])
			{
				//swap move scores
				int temp_score = move_scores[current_move];
				move_scores[current_move] = move_scores[next_move];
				move_scores[next_move] = temp_score;
				//swap moves
				int temp_move = move_list->moves[current_move];
				move_list->moves[current_move] = move_list->moves[next_move];
				move_list->moves[next_move] = temp_move;
			}
		}


	}



}

// print move scores
void print_move_scores(moves* move_list)
{
	printf(" Move scores:\n\n");

	// loop over moves within a move list
	for (int count = 0; count < move_list->count; count++)
	{
		printf(" Move: ");
		print_moves(move_list->moves[count]);
		printf(" score: %d\n", score_move(move_list->moves[count]));
	}
}





//quescence search
static inline int quiescence_search(int alpha, int beta)
{
	//communicate every 2047 nodes
	if (nodes % 2047==0)
	{
		//listen to GUI/USER
		communicate();
	}


	//increment node count
	nodes++;


	//evaluate position
	int eval = evaluate_position();

	//check if eval is greater than alpha( fail-hard beta prune)
	if (eval >= beta)
	{
		//node(move) fails high
		return beta;
	}
	//if better move
	if (eval > alpha)
	{
		//PV node(move)(PV=Principal Variation)
		alpha = eval;
	}


	//create move list
	moves move_list[1];
	//generate moves
	generate_moves(move_list);
	//sort moves
	sort_moves(move_list);


	//loop over moves
	for (int count = 0; count < move_list->count; count++)
	{
		//preserve board state
		copy_board();
		//increment ply
		ply++;

		//make legal moves
		if (make_move(move_list->moves[count], only_capture) == 0)
		{
			//decrement ply
			ply--;
			//skip to next move
			continue;

		}


		//call recursive function and score current move
		int score = -quiescence_search(-beta, -alpha);


		//decrement ply
		ply--;


		//restore board state
		restore_board();

		//return 0 if time is up
		if (stopped == 1)
		{
			return 0;
		}


		//check if score is greater than alpha( fail-hard beta prune)
		if (score >= beta)
		{
			//node(move) fails high
			return beta;
		}
		//if better move
		if (score > alpha)
		{
			//PV node(move)(PV=Principal Variation)
			alpha = score;
		}
	}
	//node(move) fails low
	return alpha;
}

//constants of LMR

const int full_depth_lmr = 4;
const int reduction_limit = 2;

//negamax search with alpha beta pruning
static inline int negamax(int alpha, int beta, int depth)
{
	//communicate every 2047 nodes
	if (nodes % 2047==0)
	{
		//listen to GUI/USER
		communicate();
	}

	//initialize PV length
	pv_length[ply] = ply;


	//check if depth is 0
	if (depth == 0)
	{
		//implement quiescence search
		return quiescence_search(alpha, beta);
	}
	//check if MAX_PLY is reached (coul be an overflow without this)
	if (ply > MAX_PLY - 1)
	{
		//return evaluation
		return evaluate_position();
	}

	//increment node count
	nodes++;
	//check if king is in check
	int in_check = is_attacked((color == white) ? get_lsb_index(bitboards[K]) : get_lsb_index(bitboards[k]), color ^ 1);
	//increase search depth if been in check
	if (in_check)
	{
		depth++;
	}
	//legal move count
	int legal_moves = 0;

	//null move pruning
	//if depth is greater than 3 and not in check
	if (depth >= 3 && in_check==0 && ply)
	{
		//preserve board state
		copy_board();

		//switch color and give opponent a chance to move
		color ^= 1;
		//reset enpassant square
		en_passant = no_square;

		//search move with reduced depth -> depth -1 - r where r is the reduction limit

		//call recursive function and score current move
		int score = -negamax(-beta, -beta + 1, depth - 1 - reduction_limit);

		restore_board();

		if (stopped == 1)
		{
			return 0;
		}

		//check if score is greater than beta( fail-hard beta prune)
		if (score >= beta)
		{
			//node(move) fails high
			return beta;
		}
	}

	//create move list
	moves move_list[1];
	//generate moves
	generate_moves(move_list);
	//if in pv node
	if(current_pv)
	{
		enable_pv_score(move_list);
	}
	//sort moves
	sort_moves(move_list);

	//moves searched in move list
	int moves_searched = 0;

	//loop over moves
	for (int count = 0; count < move_list->count; count++)
	{
		//preserve board state
		copy_board();
		//increment ply
		ply++;

		//make legal moves
		if (make_move(move_list->moves[count], all_moves) == 0)
		{
			//decrement ply
			ply--;
			//skip to next move
			continue;

		}
		//increment legal move count
		legal_moves++;

		//initialize score variable
		int score;

		// full depth search
		if (moves_searched == 0)
		{
			//normal alpha beta search
			score = -negamax(-beta, -alpha, depth - 1);
		}
		//we apply late move reduction(LMR)
		else
		{	//condition to LMR
			if (moves_searched >= full_depth_lmr && depth >= reduction_limit && !in_check && !get_capture_move(move_list->moves[count]) && !get_promotion_move(move_list->moves[count]))
			{

				//search with reduction
				score = -negamax(-alpha - 1, -alpha, depth - reduction_limit);
			}
			else
			{
				//check that full depth search is done
				score = alpha +1;
			}

			//PVS (Principal Variation Search)
			if (score > alpha)
			{
				//if score in alpha-beta window the other moves out of the window are not needed
				//adjust score(try to be as closer to alpha as it can be)
				score = -negamax(-alpha - 1, -alpha, depth - 1);


				//if we find a subsequient move that is better than the first PV moves
				if ((score > alpha) && (score < beta))
				{
					//call negamax to search again the move that has failed(previous pv search move) with normal  alpha beta window
					score = -negamax(-beta, -alpha, depth - 1);
				}
			}
		}

		//decrement ply
		ply--;


		//restore board state
		restore_board();
		//return 0 if time is up
		if (stopped == 1)
		{
			return 0;
		}

		//increment moves searched counter
		moves_searched++;

		//check if score is greater than beta( fail-hard beta prune)
		if (score >= beta)
		{
			//in quiet move
			if(get_capture_move(move_list->moves[count])==0)
			{
				//set killer move
				killers[1][ply] = killers[0][ply];
				killers[0][ply] = move_list->moves[count];
			}



			//node(move) fails high
			return beta;
		}
		//if better move
		if (score > alpha)
		{
			//set book move on quiet moves
			if (get_capture_move(move_list->moves[count]) == 0)
			{
				book_moves[get_piece_move(move_list->moves[count])][get_target_move(move_list->moves[count])] += depth;
			}




			//PV node(move)(PV=Principal Variation)
			alpha = score;

			//set PV move
			pv_table[ply][ply] = move_list->moves[count];

			//loop over next ply
			for (int next_ply = ply + 1; next_ply < pv_length[ply+1]; next_ply++)
			{
				//copy move from deeper ply to current ply
				pv_table[ply][next_ply] = pv_table[ply+1][next_ply];
			}

			//adjust PV length
			pv_length[ply] = pv_length[ply + 1];




		}
	}
	//check if no legal moves in current position
	if (legal_moves == 0)
	{
		//check if king is in check
		if (in_check)
		{
			//return checkmate score(+ply choose shortest path to mate)
			return -49000 + ply ;
		}
		else
		{
			//return stalemate score
			return 0;
		}

	}

	//node(move) fails low
	return alpha;
}



//search position for best move
void search_position(int depth)
{

	//initialize score
	int score = 0;

	//initialize time up flag
	stopped = 0;

	//clear nodes count
	nodes = 0;
	//initialize current PV flag
	current_pv = 0;
	//initialize score PV move
	score_pv = 0;

	//clear support data structures for search(book moves, killer moves,PV table,PV length) for improving search speed
	memset(killers, 0, sizeof(killers));
	memset(book_moves, 0, sizeof(book_moves));
	memset(pv_table, 0, sizeof(pv_table));
	memset(pv_length, 0, sizeof(pv_length));


	//initialize alpha and beta
	int alpha = -50000;
	int beta = 50000;

	//iteratively deepening search

	//loop over depth
	for (int current_depth = 1; current_depth <= depth; current_depth++)
	{



		//enable PV flag
		current_pv = 1;

		//find best move in current position
		score = negamax(-50000, 50000, current_depth);
		//if time is up
		if (stopped == 1)
		{
			//stop engine and return current best move
			break;
		}

		// if score is outcolor the window, try again with a full-width aspiration window and same depth
		if ((score <= alpha) || (score >= beta)) {
			alpha = -50000;
			beta = 50000;
			continue;
		}

		// adjust aspiration window
		alpha = score - 50;
		beta = score + 50;


		printf("info score cp %d depth %d nodes %ld PV: ", score, current_depth, nodes);

		//loop over principal variation
		for (int count = 0; count < pv_length[0]; count++)
		{
			//print PV moves
			print_moves(pv_table[0][count]);
			printf(" ");
		}
		printf("\n");
	}



	printf("bestmove ");
	print_moves(pv_table[0][0]);
	printf("\n");

}







/*****************************/
/*       UCI Conection       */
/*****************************/

//parse move command (USER/GUI string move input---> "a7a8n")
int parse_move(char* move_string)
{

	//create move list
	moves move_list[1];
	//generate moves
	generate_moves(move_list);




	//parse move
	int source_square = (move_string[0] - 'a') + (8-(move_string[1] - '0')) * 8;
	int target_square = (move_string[2] - 'a') + (8-(move_string[3] - '0')) * 8;


	//loop over moves
	for (int move_count = 0; move_count < move_list->count; move_count++)
	{
		//initialize move
		int move = move_list->moves[move_count];



		//check if move is valid
		if (get_source_move(move) == source_square && get_target_move(move) == target_square)

		{	//promote piece
			int promotion = get_promotion_move(move);

			//check if promotion
			if (promotion)
			{

				if ((promotion == Q || promotion == q) && move_string[4] == 'q')
				{
					return move;
				}
				else if ((promotion == R || promotion == r) && move_string[4] == 'r')
				{
					return move;
				}
				else if ((promotion == B || promotion == b) && move_string[4] == 'b')
				{
					return move;
				}
				else if ((promotion == N || promotion == n) && move_string[4] == 'n')
				{
					return move;
				}

				//continue the next move on wrong promotions
				continue;

			}
			//legal move
			return move;
		}

	}
	//illegal move
	return 0;
}


//parse UCI position command
void parse_position(char* command)
{
	//shif pointer to next token
	command += 9;

	//initialize pointer to current character
	char* current_char = command;

	//parse UCI startpos
	if (strncmp(command, "startpos", 8) == 0)
	{
		//initialize start position
		parse_fen(start_position);
	}
	//parse UCI fen
	else
	{
		//check that fen command is available
		current_char = strstr(command, "fen");
		//if no fen command
		if (!current_char)
		{
			parse_fen(start_position);
		}

		//found fen
		else
		{
			//skip to next token
			current_char += 4;
			//parse fen string and init chess board with it
			parse_fen(current_char);

		}

	}
	//parse UCI moves command after fen position
	current_char = strstr(command, "moves");

	//moves command
	if (current_char)
	{
		//skip to next token
		current_char += 6;
		//loop over moves
		while (*current_char)
		{
			//parse move
			int move = parse_move(current_char);
			//check if move is valid
			if (move)
			{
				make_move(move, all_moves);

			}
			else
			{
				printf("Illegal move: %s\n", current_char);
				break;
			}
			//move pointer to end of current move
			while (*current_char && *current_char != ' ')
			{
				current_char++;
			}
			//go to next move(skip space)
			current_char++;

		}

	}
	//print board
	print_board();
}
//parse UCI go command
void parse_go(char* command)
{
	//initilize depth
	int depth = -1;
	//initialize pointer to current depth
	char* input_var = NULL;

	// infinite search
	if ((input_var = strstr(command,"infinite"))) {}

	// match UCI "binc" command
	if ((input_var = strstr(command,"binc")) && color == black)
		// parse black time increment
			inc = atoi(input_var + 5);

	// match UCI "winc" command
	if ((input_var = strstr(command,"winc")) && color == white)
		// parse white time increment
			inc = atoi(input_var + 5);

	// match UCI "wtime" command
	if ((input_var = strstr(command,"wtime")) && color == white)
		// parse white time limit
			time_var = atoi(input_var + 6);

	// match UCI "btime" command
	if ((input_var = strstr(command,"btime")) && color == black)
		// parse black time limit
			time_var = atoi(input_var + 6);

	// match UCI "movestogo" command
	if ((input_var = strstr(command,"movestogo")))
		// parse number of moves to go
			movestogo = atoi(input_var + 10);

	// match UCI "movetime" command
	if ((input_var = strstr(command,"movetime")))
		// parse amount of time allowed to spend to make a move
			movetime = atoi(input_var + 9);
	//set depth search
	if(input_var = strstr(command,"depth"))
		//convert to int and set depth
		depth = atoi(input_var + 6);
	// if move time is not available
	if(movetime != -1)
	{
		// set time equal to move time
		time_var = movetime;

		// set moves to go to 1
		movestogo = 1;
	}

	// init start time
	starttime = get_time_ms();

	// init search depth
	depth = depth;

	// if time control is available
	if(time_var != -1)
	{
		// flag we're playing with time control
		timeset = 1;

		// set up timing
		time_var /= movestogo;
		time_var -= 50;
		stoptime = starttime + time_var + inc;
	}

	// if depth is not available
	if(depth == -1)
		// set depth to 64 plies (takes ages to complete...)
			depth = 64;

	// print debug info
	printf("time:%d start:%d stop:%d depth:%d timeset:%d\n",
	time_var, starttime, stoptime, depth, timeset);

	// search position
	search_position(depth);
}


//GUI-> isready     /////  Engine -> readyok
// GUI-> ucinewgame
//main UCI function
void uci_loop()
{

	//initialize user/gui input buffer
	char input_buffer[2000];


	//reset stdin and stdout buffering

	setvbuf(stdout, NULL, _IOFBF,sizeof(stdout));
	setvbuf(stdin, NULL, _IOFBF,sizeof(stdin));

	//print engine info
	printf("id name Chess Engine\n");
	printf("id author alvjimper\n");
	printf("uciok\n");



	//loop until exit
	while (1)
	{
		//reset user input
		memset(input_buffer, 0, sizeof(input_buffer));
		//empty stdout buffer
		fflush(stdout);
		//get user input


		//parse input
		if (!fgets(input_buffer, sizeof(input_buffer), stdin))
		{
			//continue loop
			continue;
		}
		//input is avalable
		if(input_buffer[0]== '\n')
		{
			//continue loop
			continue;
		}
		//parse isready command
		if (strncmp(input_buffer, "isready", 7) == 0)
		{
			printf("readyok\n");
			continue;
		}
		//parse position command
		else if (strncmp(input_buffer, "position", 8) == 0)
		{
			parse_position(input_buffer);

		}
		//parse ucinewgame command
		else if (strncmp(input_buffer, "ucinewgame", 10) == 0)
		{
			parse_position("position startpos");
		}

		//parse go command
		else if (strncmp(input_buffer, "go", 2) == 0)
		{
			parse_go(input_buffer);

		}
		//parse quit command
		else if (strncmp(input_buffer, "quit", 4) == 0)
		{
			printf("bye\n");
			break;
		}
		//parse uci command
		else if (strncmp(input_buffer, "uci", 3) == 0)
		{
			//print engine info
			printf("id name Chess Engine\n");
			printf("id author alvjimper\n");
			printf("uciok\n");
			continue;
		}



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
	//initialize pieces attacks
	init_pieces_attacks(bishop);
	init_pieces_attacks(rook);

	//initialize random keys
	init_random_keys();
}
int main()
{

	//initialize all
	init_all();

	//debug mode variable
	int debug_mode = 1;

	//if debug mode is on
	if (debug_mode)
	{
		parse_fen(start_position);
		print_board();
		clear_transposition_table();

		record_hash_to_tt(45,1,hash_flag_beta);
		int score = read_hash_tt(20,30,1);

		//print hash entry score
		printf("score from hash entry: %d\n", score);


	}
	else
	{
		//connect to GUI
		uci_loop();
	}






	return 0;
}