#ifndef INC_SPINLOCK_H
#define INC_SPINLOCK_H

int sys_spin_lock_init(const char *name);
void sys_spin_lock(int lk);
void sys_spin_unlock(int lk);

#endif /* INC_SPINLOCK_H */
