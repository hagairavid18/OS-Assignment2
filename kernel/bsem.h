#define MAX_BSEM 128

int bsem_alloc(void);

void bsem_free(int);

void bsem_down(int);

void bsem_up(int);