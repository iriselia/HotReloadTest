#define M 1

#if 0 don't do this'
#error 1
#endif

#if defined N | defined M
#error 2
#endif

#if defined(M) M
#error 3
#endif



#if - -1 - 1
#error
#endif



#define a
#if (1)
struct b;
#endif



int main()
{
	return 0;
}