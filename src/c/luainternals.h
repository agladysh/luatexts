/*
* luainternals.h
* Code quoted from MIT-licensed Lua 5.1.4 internals
* See copyright notice in lua.h
*/

#ifndef LUATEXTS_LUAINTERNALS_H_INCLUDED_
#define LUATEXTS_LUAINTERNALS_H_INCLUDED_

/*
* BEGIN COPY-PASTE FROM Lua 5.1.4 luaconf.h
* WARNING: If your Lua config differs, fix this!
*/

#define luai_numeq(a,b)		((a)==(b))
#define luai_numisnan(a)	(!luai_numeq((a), (a)))

/*
* END COPY-PASTE FROM Lua 5.1.4 luaconf.h
*/

/*
* BEGIN COPY-PASTE FROM Lua 5.1.4 lobject.h
*/

int luaO_log2 (unsigned int x);

#define ceillog2(x)       (luaO_log2((x)-1) + 1)

/*
* END COPY-PASTE FROM Lua 5.1.4 lobject.h
*/

/*
* BEGIN COPY-PASTE FROM Lua 5.1.4 ltable.c
*/

#ifdef LUAI_BITSINT
/*
** max size of array part is 2^MAXBITS
*/
#if LUAI_BITSINT > 26
#define MAXBITS		26
#else
#define MAXBITS		(LUAI_BITSINT-2)
#endif

/*
* END COPY-PASTE FROM Lua 5.1.4 ltable.c
*/

#else
/* LuaJIT does not have LUAI_BITSINT defined */
#define MAXBITS		26
#endif

/*
* BEGIN COPY-PASTE FROM Lua 5.1.4 ltable.c
*/

#define MAXASIZE	(1 << MAXBITS)

/*
* END COPY-PASTE FROM Lua 5.1.4 ltable.c
*/

#endif /* LUATEXTS_LUAINTERNALS_H_INCLUDED_ */
