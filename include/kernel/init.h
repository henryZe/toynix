#ifndef KERN_INIT_H
#define KERN_INIT_H
#ifndef TOYNIX_KERNEL
# error "This is a Toynix kernel header; user programs should not #include it"
#endif

void init(void);
void mp_main(void);

#endif // !KERN_INIT_H
