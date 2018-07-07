#ifndef KERN_TIME_H
#define KERN_TIME_H
#ifndef TOYNIX_KERNEL
# error "This is a Toynix kernel header; user programs should not #include it"
#endif

void time_init(void);
void time_tick(void);
unsigned int time_msec(void);

#endif /* KERN_TIME_H */
