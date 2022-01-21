#ifdef DEBUG
# define DEBUG_PRINT(s) printf s
#else
# define DEBUG_PRINT(s) do {} while (0)
#endif