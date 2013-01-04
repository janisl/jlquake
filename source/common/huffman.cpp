//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	This is based on the Adaptive Huffman algorithm described in
//**  Sayood's Data Compression book.  The ranks are not actually stored,
//**  but implicitly defined by the location of a node within a
//**  doubly-linked list
//**
//**************************************************************************

#include "huffman.h"
#include "Common.h"

static int bloc = 0;

void Huff_putBit(int bit, byte* fout, int* offset)
{
	bloc = *offset;
	int x = bloc >> 3;
	int y = bloc & 7;
	if (!y)
	{
		fout[x] = 0;
	}
	fout[x] |= bit << y;
	bloc++;
	*offset = bloc;
}

int Huff_getBit(byte* fin, int* offset)
{
	bloc = *offset;
	int t = (fin[(bloc >> 3)] >> (bloc & 7)) & 0x1;
	bloc++;
	*offset = bloc;
	return t;
}

//	Add a bit to the output file (buffered)
static void add_bit(char bit, byte* fout)
{
	int y = bloc >> 3;
	int x = bloc++ & 7;
	if (x == 0)
	{
		fout[y] = 0;
	}
	fout[y] |= bit << x;
}

// Receive one bit from the input file (buffered)
static int get_bit(byte* fin)
{
	int t = (fin[(bloc >> 3)] >> (bloc & 7)) & 0x1;
	bloc++;
	return t;
}

static node_t** get_ppnode(huff_t* huff)
{
	node_t** tppnode;
	if (!huff->freelist)
	{
		return &(huff->nodePtrs[huff->blocPtrs++]);
	}
	else
	{
		tppnode = huff->freelist;
		huff->freelist = (node_t**)*tppnode;
		return tppnode;
	}
}

static void free_ppnode(huff_t* huff, node_t** ppnode)
{
	*ppnode = (node_t*)huff->freelist;
	huff->freelist = ppnode;
}

// Swap the location of these two nodes in the tree
static void swap(huff_t* huff, node_t* node1, node_t* node2)
{
	node_t* par1 = node1->parent;
	node_t* par2 = node2->parent;

	if (par1)
	{
		if (par1->left == node1)
		{
			par1->left = node2;
		}
		else
		{
			par1->right = node2;
		}
	}
	else
	{
		huff->tree = node2;
	}

	if (par2)
	{
		if (par2->left == node2)
		{
			par2->left = node1;
		}
		else
		{
			par2->right = node1;
		}
	}
	else
	{
		huff->tree = node1;
	}

	node1->parent = par2;
	node2->parent = par1;
}

// Swap these two nodes in the linked list (update ranks)
static void swaplist(node_t* node1, node_t* node2)
{
	node_t* par1 = node1->next;
	node1->next = node2->next;
	node2->next = par1;

	par1 = node1->prev;
	node1->prev = node2->prev;
	node2->prev = par1;

	if (node1->next == node1)
	{
		node1->next = node2;
	}
	if (node2->next == node2)
	{
		node2->next = node1;
	}
	if (node1->next)
	{
		node1->next->prev = node1;
	}
	if (node2->next)
	{
		node2->next->prev = node2;
	}
	if (node1->prev)
	{
		node1->prev->next = node1;
	}
	if (node2->prev)
	{
		node2->prev->next = node2;
	}
}

// Do the increments
static void increment(huff_t* huff, node_t* node)
{
	if (!node)
	{
		return;
	}

	if (node->next != NULL && node->next->weight == node->weight)
	{
		node_t* lnode = *node->head;
		if (lnode != node->parent)
		{
			swap(huff, lnode, node);
		}
		swaplist(lnode, node);
	}
	if (node->prev && node->prev->weight == node->weight)
	{
		*node->head = node->prev;
	}
	else
	{
		*node->head = NULL;
		free_ppnode(huff, node->head);
	}
	node->weight++;
	if (node->next && node->next->weight == node->weight)
	{
		node->head = node->next->head;
	}
	else
	{
		node->head = get_ppnode(huff);
		*node->head = node;
	}
	if (node->parent)
	{
		increment(huff, node->parent);
		if (node->prev == node->parent)
		{
			swaplist(node, node->parent);
			if (*node->head == node)
			{
				*node->head = node->parent;
			}
		}
	}
}

void Huff_addRef(huff_t* huff, byte ch)
{
	if (huff->loc[ch] == NULL)	/* if this is the first transmission of this node */
	{
		node_t* tnode = &(huff->nodeList[huff->blocNode++]);
		node_t* tnode2 = &(huff->nodeList[huff->blocNode++]);

		tnode2->symbol = INTERNAL_NODE;
		tnode2->weight = 1;
		tnode2->next = huff->lhead->next;
		if (huff->lhead->next)
		{
			huff->lhead->next->prev = tnode2;
			if (huff->lhead->next->weight == 1)
			{
				tnode2->head = huff->lhead->next->head;
			}
			else
			{
				tnode2->head = get_ppnode(huff);
				*tnode2->head = tnode2;
			}
		}
		else
		{
			tnode2->head = get_ppnode(huff);
			*tnode2->head = tnode2;
		}
		huff->lhead->next = tnode2;
		tnode2->prev = huff->lhead;

		tnode->symbol = ch;
		tnode->weight = 1;
		tnode->next = huff->lhead->next;
		if (huff->lhead->next)
		{
			huff->lhead->next->prev = tnode;
			if (huff->lhead->next->weight == 1)
			{
				tnode->head = huff->lhead->next->head;
			}
			else
			{
				/* this should never happen */
				tnode->head = get_ppnode(huff);
				*tnode->head = tnode2;
			}
		}
		else
		{
			/* this should never happen */
			tnode->head = get_ppnode(huff);
			*tnode->head = tnode;
		}
		huff->lhead->next = tnode;
		tnode->prev = huff->lhead;
		tnode->left = tnode->right = NULL;

		if (huff->lhead->parent)
		{
			if (huff->lhead->parent->left == huff->lhead)	/* lhead is guaranteed to by the NYT */
			{
				huff->lhead->parent->left = tnode2;
			}
			else
			{
				huff->lhead->parent->right = tnode2;
			}
		}
		else
		{
			huff->tree = tnode2;
		}

		tnode2->right = tnode;
		tnode2->left = huff->lhead;

		tnode2->parent = huff->lhead->parent;
		huff->lhead->parent = tnode->parent = tnode2;

		huff->loc[ch] = tnode;

		increment(huff, tnode2->parent);
	}
	else
	{
		increment(huff, huff->loc[ch]);
	}
}

// Get a symbol
int Huff_Receive(node_t* node, int* ch, byte* fin)
{
	while (node && node->symbol == INTERNAL_NODE)
	{
		if (get_bit(fin))
		{
			node = node->right;
		}
		else
		{
			node = node->left;
		}
	}
	if (!node)
	{
		return 0;
//		common->Error("Illegal tree!\n");
	}
	return (*ch = node->symbol);
}

// Get a symbol
void Huff_offsetReceive(node_t* node, int* ch, byte* fin, int* offset)
{
	bloc = *offset;
	while (node && node->symbol == INTERNAL_NODE)
	{
		if (get_bit(fin))
		{
			node = node->right;
		}
		else
		{
			node = node->left;
		}
	}
	if (!node)
	{
		*ch = 0;
		return;
//		common->Error("Illegal tree!\n");
	}
	*ch = node->symbol;
	*offset = bloc;
}

// Send the prefix code for this node
static void send(node_t* node, node_t* child, byte* fout)
{
	if (node->parent)
	{
		send(node->parent, node, fout);
	}
	if (child)
	{
		if (node->right == child)
		{
			add_bit(1, fout);
		}
		else
		{
			add_bit(0, fout);
		}
	}
}

// Send a symbol
void Huff_transmit(huff_t* huff, int ch, byte* fout)
{
	if (huff->loc[ch] == NULL)
	{
		/* node_t hasn't been transmitted, send a NYT, then the symbol */
		Huff_transmit(huff, NYT, fout);
		for (int i = 7; i >= 0; i--)
		{
			add_bit((char)((ch >> i) & 0x1), fout);
		}
	}
	else
	{
		send(huff->loc[ch], NULL, fout);
	}
}

void Huff_offsetTransmit(huff_t* huff, int ch, byte* fout, int* offset)
{
	bloc = *offset;
	send(huff->loc[ch], NULL, fout);
	*offset = bloc;
}

void Huff_Init(huffman_t* huff)
{
	Com_Memset(&huff->compressor, 0, sizeof(huff_t));
	Com_Memset(&huff->decompressor, 0, sizeof(huff_t));

	// Initialize the tree & list with the NYT node
	huff->decompressor.tree = huff->decompressor.lhead = huff->decompressor.ltail = huff->decompressor.loc[NYT] = &(huff->decompressor.nodeList[huff->decompressor.blocNode++]);
	huff->decompressor.tree->symbol = NYT;
	huff->decompressor.tree->weight = 0;
	huff->decompressor.lhead->next = huff->decompressor.lhead->prev = NULL;
	huff->decompressor.tree->parent = huff->decompressor.tree->left = huff->decompressor.tree->right = NULL;

	// Add the NYT (not yet transmitted) node into the tree/list */
	huff->compressor.tree = huff->compressor.lhead = huff->compressor.loc[NYT] =  &(huff->compressor.nodeList[huff->compressor.blocNode++]);
	huff->compressor.tree->symbol = NYT;
	huff->compressor.tree->weight = 0;
	huff->compressor.lhead->next = huff->compressor.lhead->prev = NULL;
	huff->compressor.tree->parent = huff->compressor.tree->left = huff->compressor.tree->right = NULL;
	huff->compressor.loc[NYT] = huff->compressor.tree;
}

void Huff_Decompress(QMsg* mbuf, int offset)
{
	int ch, cch, i, j, size;
	byte seq[65536];
	byte* buffer;
	huff_t huff;

	size = mbuf->cursize - offset;
	buffer = mbuf->_data + offset;

	if (size <= 0)
	{
		return;
	}

	Com_Memset(&huff, 0, sizeof(huff_t));
	// Initialize the tree & list with the NYT node
	huff.tree = huff.lhead = huff.ltail = huff.loc[NYT] = &(huff.nodeList[huff.blocNode++]);
	huff.tree->symbol = NYT;
	huff.tree->weight = 0;
	huff.lhead->next = huff.lhead->prev = NULL;
	huff.tree->parent = huff.tree->left = huff.tree->right = NULL;

	cch = buffer[0] * 256 + buffer[1];
	// don't overflow with bad messages
	if (cch > mbuf->maxsize - offset)
	{
		cch = mbuf->maxsize - offset;
	}
	bloc = 16;

	for (j = 0; j < cch; j++)
	{
		ch = 0;
		// don't overflow reading from the messages
		// FIXME: would it be better to have a overflow check in get_bit ?
		if ((bloc >> 3) > size)
		{
			seq[j] = 0;
			break;
		}
		Huff_Receive(huff.tree, &ch, buffer);				/* Get a character */
		if (ch == NYT)									/* We got a NYT, get the symbol associated with it */
		{
			ch = 0;
			for (i = 0; i < 8; i++)
			{
				ch = (ch << 1) + get_bit(buffer);
			}
		}

		seq[j] = ch;									/* Write symbol */

		Huff_addRef(&huff, (byte)ch);								/* Increment node */
	}
	mbuf->cursize = cch + offset;
	Com_Memcpy(mbuf->_data + offset, seq, cch);
}

void Huff_Compress(QMsg* mbuf, int offset)
{
	int i, ch, size;
	byte seq[65536];
	byte* buffer;
	huff_t huff;

	size = mbuf->cursize - offset;
	buffer = mbuf->_data + +offset;

	if (size <= 0)
	{
		return;
	}

	Com_Memset(&huff, 0, sizeof(huff_t));
	// Add the NYT (not yet transmitted) node into the tree/list */
	huff.tree = huff.lhead = huff.loc[NYT] =  &(huff.nodeList[huff.blocNode++]);
	huff.tree->symbol = NYT;
	huff.tree->weight = 0;
	huff.lhead->next = huff.lhead->prev = NULL;
	huff.tree->parent = huff.tree->left = huff.tree->right = NULL;
	huff.loc[NYT] = huff.tree;

	seq[0] = (size >> 8);
	seq[1] = size & 0xff;

	bloc = 16;

	for (i = 0; i < size; i++)
	{
		ch = buffer[i];
		Huff_transmit(&huff, ch, seq);						/* Transmit symbol */
		Huff_addRef(&huff, (byte)ch);								/* Do update */
	}

	bloc += 8;												// next byte

	mbuf->cursize = (bloc >> 3) + offset;
	Com_Memcpy(mbuf->_data + offset, seq, (bloc >> 3));
}

struct huffnode_t
{
	huffnode_t* zero;
	huffnode_t* one;
	unsigned char val;
	float freq;
};

struct hufftab_t
{
	unsigned int bits;
	int len;
};

static huffnode_t* HuffTree = 0;
static hufftab_t HuffLookup[256];

static float HuffFreq[256] =
{
	0.14473691,
	0.01147017,
	0.00167522,
	0.03831121,
	0.00356579,
	0.03811315,
	0.00178254,
	0.00199644,
	0.00183511,
	0.00225716,
	0.00211240,
	0.00308829,
	0.00172852,
	0.00186608,
	0.00215921,
	0.00168891,
	0.00168603,
	0.00218586,
	0.00284414,
	0.00161833,
	0.00196043,
	0.00151029,
	0.00173932,
	0.00218370,
	0.00934121,
	0.00220530,
	0.00381211,
	0.00185456,
	0.00194675,
	0.00161977,
	0.00186680,
	0.00182071,
	0.06421956,
	0.00537786,
	0.00514019,
	0.00487155,
	0.00493925,
	0.00503143,
	0.00514019,
	0.00453520,
	0.00454241,
	0.00485642,
	0.00422407,
	0.00593387,
	0.00458130,
	0.00343687,
	0.00342823,
	0.00531592,
	0.00324890,
	0.00333388,
	0.00308613,
	0.00293776,
	0.00258918,
	0.00259278,
	0.00377105,
	0.00267488,
	0.00227516,
	0.00415997,
	0.00248763,
	0.00301555,
	0.00220962,
	0.00206990,
	0.00270369,
	0.00231694,
	0.00273826,
	0.00450928,
	0.00384380,
	0.00504728,
	0.00221251,
	0.00376961,
	0.00232990,
	0.00312574,
	0.00291688,
	0.00280236,
	0.00252436,
	0.00229461,
	0.00294353,
	0.00241201,
	0.00366590,
	0.00199860,
	0.00257838,
	0.00225860,
	0.00260646,
	0.00187256,
	0.00266552,
	0.00242641,
	0.00219450,
	0.00192082,
	0.00182071,
	0.02185930,
	0.00157439,
	0.00164353,
	0.00161401,
	0.00187544,
	0.00186248,
	0.03338637,
	0.00186968,
	0.00172132,
	0.00148509,
	0.00177749,
	0.00144620,
	0.00192442,
	0.00169683,
	0.00209439,
	0.00209439,
	0.00259062,
	0.00194531,
	0.00182359,
	0.00159096,
	0.00145196,
	0.00128199,
	0.00158376,
	0.00171412,
	0.00243433,
	0.00345704,
	0.00156359,
	0.00145700,
	0.00157007,
	0.00232342,
	0.00154198,
	0.00140730,
	0.00288807,
	0.00152830,
	0.00151246,
	0.00250203,
	0.00224420,
	0.00161761,
	0.00714383,
	0.08188576,
	0.00802537,
	0.00119484,
	0.00123805,
	0.05632671,
	0.00305156,
	0.00105584,
	0.00105368,
	0.00099246,
	0.00090459,
	0.00109473,
	0.00115379,
	0.00261223,
	0.00105656,
	0.00124381,
	0.00100326,
	0.00127550,
	0.00089739,
	0.00162481,
	0.00100830,
	0.00097229,
	0.00078864,
	0.00107240,
	0.00084409,
	0.00265760,
	0.00116891,
	0.00073102,
	0.00075695,
	0.00093916,
	0.00106880,
	0.00086786,
	0.00185600,
	0.00608367,
	0.00133600,
	0.00075695,
	0.00122077,
	0.00566955,
	0.00108249,
	0.00259638,
	0.00077063,
	0.00166586,
	0.00090387,
	0.00087074,
	0.00084914,
	0.00130935,
	0.00162409,
	0.00085922,
	0.00093340,
	0.00093844,
	0.00087722,
	0.00108249,
	0.00098598,
	0.00095933,
	0.00427593,
	0.00496661,
	0.00102775,
	0.00159312,
	0.00118404,
	0.00114947,
	0.00104936,
	0.00154342,
	0.00140082,
	0.00115883,
	0.00110769,
	0.00161112,
	0.00169107,
	0.00107816,
	0.00142747,
	0.00279804,
	0.00085922,
	0.00116315,
	0.00119484,
	0.00128559,
	0.00146204,
	0.00130215,
	0.00101551,
	0.00091756,
	0.00161184,
	0.00236375,
	0.00131872,
	0.00214120,
	0.00088875,
	0.00138570,
	0.00211960,
	0.00094060,
	0.00088083,
	0.00094564,
	0.00090243,
	0.00106160,
	0.00088659,
	0.00114514,
	0.00095861,
	0.00108753,
	0.00124165,
	0.00427016,
	0.00159384,
	0.00170547,
	0.00104431,
	0.00091395,
	0.00095789,
	0.00134681,
	0.00095213,
	0.00105944,
	0.00094132,
	0.00141883,
	0.00102127,
	0.00101911,
	0.00082105,
	0.00158448,
	0.00102631,
	0.00087938,
	0.00139290,
	0.00114658,
	0.00095501,
	0.00161329,
	0.00126542,
	0.00113218,
	0.00123661,
	0.00101695,
	0.00112930,
	0.00317976,
	0.00085346,
	0.00101190,
	0.00189849,
	0.00105728,
	0.00186824,
	0.00092908,
	0.00160896,
};

void FindTab(huffnode_t* tmp, int len, unsigned int bits)
{
	if (!tmp)
	{
		common->FatalError("no huff node");
	}
	if (tmp->zero)
	{
		if (!tmp->one)
		{
			common->FatalError("no one in node");
		}
		if (len >= 32)
		{
			common->FatalError("compression screwd");
		}
		FindTab(tmp->zero, len + 1, bits << 1);
		FindTab(tmp->one, len + 1, (bits << 1) | 1);
		return;
	}
	HuffLookup[tmp->val].len = len;
	HuffLookup[tmp->val].bits = bits;
	return;
}

static unsigned char Masks[8] =
{
	0x1,
	0x2,
	0x4,
	0x8,
	0x10,
	0x20,
	0x40,
	0x80
};

void PutBit(unsigned char* buf,int pos,int bit)
{
	if (bit)
	{
		buf[pos / 8] |= Masks[pos % 8];
	}
	else
	{
		buf[pos / 8] &= ~Masks[pos % 8];
	}
}

int GetBit(unsigned char* buf,int pos)
{
	if (buf[pos / 8] & Masks[pos % 8])
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void BuildTree(float* freq)
{
	huffnode_t* work[256];
	for (int i = 0; i < 256; i++)
	{
		work[i] = (huffnode_t*)Mem_Alloc(sizeof(huffnode_t));
		work[i]->val = (unsigned char)i;
		work[i]->freq = freq[i];
		work[i]->zero = 0;
		work[i]->one = 0;
		HuffLookup[i].len = 0;
	}
	huffnode_t* tmp;
	for (int i = 0; i < 255; i++)
	{
		int minat1 = -1;
		int minat2 = -1;
		float min1 = 1E30;
		float min2 = 1E30;
		for (int j = 0; j < 256; j++)
		{
			if (!work[j])
			{
				continue;
			}
			if (work[j]->freq < min1)
			{
				minat2 = minat1;
				min2 = min1;
				minat1 = j;
				min1 = work[j]->freq;
			}
			else if (work[j]->freq < min2)
			{
				minat2 = j;
				min2 = work[j]->freq;
			}
		}
		if (minat1 < 0)
		{
			common->FatalError("minatl: %d", minat1);
		}
		if (minat2 < 0)
		{
			common->FatalError("minat2: %d", minat2);
		}
		tmp = (huffnode_t*)Mem_Alloc(sizeof(huffnode_t));
		tmp->zero = work[minat2];
		tmp->one = work[minat1];
		tmp->freq = work[minat2]->freq + work[minat1]->freq;
		tmp->val = 0xff;
		work[minat1] = tmp;
		work[minat2] = 0;
	}
	HuffTree = tmp;
	FindTab(HuffTree, 0, 0);
}

void HuffInit()
{
	BuildTree(HuffFreq);
}

void HuffDecode(unsigned char* in, unsigned char* out, int inlen, int* outlen)
{
	if (*in == 0xff)
	{
		Com_Memcpy(out,in + 1,inlen - 1);
		*outlen = inlen - 1;
		return;
	}
	int tbits = (inlen - 1) * 8 - *in;
	int bits = 0;
	*outlen = 0;
	while (bits < tbits)
	{
		huffnode_t* tmp = HuffTree;
		do
		{
			if (GetBit(in + 1,bits))
			{
				tmp = tmp->one;
			}
			else
			{
				tmp = tmp->zero;
			}
			bits++;
		}
		while (tmp->zero);
		*out++ = tmp->val;
		(*outlen)++;
	}
}

void HuffEncode(unsigned char* in,unsigned char* out,int inlen,int* outlen)
{
	int bitat = 0;
	for (int i = 0; i < inlen; i++)
	{
		unsigned int t = HuffLookup[in[i]].bits;
		for (int j = 0; j < HuffLookup[in[i]].len; j++)
		{
			PutBit(out + 1,bitat + HuffLookup[in[i]].len - j - 1, t & 1);
			t >>= 1;
		}
		bitat += HuffLookup[in[i]].len;
	}
	*outlen = 1 + (bitat + 7) / 8;
	*out = 8 * ((*outlen) - 1) - bitat;
	if (*outlen >= inlen + 1)
	{
		*out = 0xff;
		Com_Memcpy(out + 1, in, inlen);
		*outlen = inlen + 1;
	}
}
