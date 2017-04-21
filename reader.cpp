#include <cstdio>

struct CREC {
    int       word1;
    int       word2;
    double    val;
};


int main()
{
    CREC rec;

    while ( fread(&rec, sizeof(CREC), 1, stdin) == 1 )
        printf("%d\t%d\t%lf\n", rec.word1, rec.word2, rec.val);

    return 0;
}
