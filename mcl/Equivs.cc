/***************************************************************************************[Equivs.cc]
Copyright (c) 2011, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "minisat/mtl/Alg.h"
#include "mcl/Equivs.h"
// TMP:
#include "mcl/CircPrelude.h"

using namespace Minisat;

// TODO: use path compression in merge.
bool Equivs::merge(Sig x, Sig y)
{
    if (!ok) return false;

    assert(x != sig_Undef);
    assert(y != sig_Undef);

    // printf(" ... merging[1]: x="); printSig(x); printf(", y="); printSig(y); printf("\n");

    x = leader(x);
    y = leader(y);

    if (y < x)  { Sig tmp = x; x = y; y = tmp; } // Order (useful?).
    if (sign(x)){ x = ~x; y = ~y; }              // Make 'x' unsigned.
    if (x == ~y) return ok = false;              // Tried to merge 'x' with '~x'.
    if (x ==  y) return true;                    // Merge 'x' with 'x' is redundant.

    assert(x < y);

    // printf(" ... merging[2]: x="); printSig(x); printf(", y="); printSig(y); printf("\n");

    // Map 'y' to 'x' while handling signs:
    union_find.growTo(gate(y), sig_Undef);
    union_find[gate(y)] = x ^ sign(y);

    // Create the class for 'x' if needed:
    class_map.growTo(gate(x), class_Undef);
    ClassId& xid = class_map[gate(x)];
    if (xid == class_Undef){
        classes.push();
        xid = classes.size()-1;
        classes[xid].push(x);
    }

    // Extend the class for 'x' with all elements of 'y':
    if (!class_map.has(gate(y)) || class_map[gate(y)] == class_Undef)
        // Just append 'y' to 'x' previous class:
        classes[xid].push(y);
    else{
        // Append all of 'y's elements to 'x' with potentially inverted sign:
        ClassId& yid = class_map[gate(y)];
        for (int i = 0; i < classes[yid].size(); i++)
            classes[xid].push(classes[yid][i] ^ sign(y));

        // Free the class-vector for 'y':
        ClassId final = classes.size()-1;
        assert(classes.size() > 0);
        assert(yid < (uint32_t)classes.size());
        if (final > yid){
            assert(!sign(classes[final][0]));
            class_map[gate(classes[final][0])] = yid;
            classes[final].moveTo(classes[yid]);
        }
        classes.pop();
        yid = class_Undef;
    }

    assert(!sign(classes[xid][0]));
    assert(classes[xid][0] == x);

    return true;
}


void Equivs::clear(bool dealloc)
{
    union_find.clear(dealloc);
    class_map .clear(dealloc);
    classes   .clear(dealloc);
}


void Equivs::moveTo(Equivs& to)
{
    union_find.moveTo(to.union_find);
    class_map .moveTo(to.class_map);
    classes   .moveTo(to.classes);
    to.ok = ok;
}
