/*
 *  (C) 2010 by Computer System Laboratory, IIS, Academia Sinica, Taiwan.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "exec-all.h"
#include "tcg-op.h"
#include "helper.h"
#define GEN_HELPER 1
#include "helper.h"
#include "optimization.h"

extern uint8_t *optimization_ret_addr;

/*
 * Shadow Stack
 */

static inline void shack_init(CPUState *env)
{
    env->shack = env->shack_top = (void *)malloc(SHACK_SIZE * sizeof(void*));
    env->shack_end = env->shack + SHACK_SIZE*sizeof(void*);
    env->shadow_hash_list = (struct hash_list *)calloc(1, sizeof(struct hash_list));
}

struct shadow_pair* find_hash_pair(CPUState *env, target_ulong next_eip)
{
    int key = next_eip & SHACK_MASK;
    struct shadow_pair *pair = env->shadow_hash_list->head[key];
    while(pair != NULL){
        if(pair->guest_eip == next_eip) return pair;
        pair = pair->nxt;
    }

    struct shadow_pair *new_pair = (struct shadow_pair*)malloc(sizeof(struct shadow_pair));
    new_pair->guest_eip = next_eip;
    new_pair->host_eip = NULL;
    new_pair->nxt = env->shadow_hash_list->head[key];
    env->shadow_hash_list->head[key] = new_pair;
    return new_pair;
    
}

/*
 * shack_set_shadow()
 *  Insert a guest eip to host eip pair if it is not yet created.
 */
void shack_set_shadow(CPUState *env, target_ulong guest_eip, unsigned long *host_eip)
{
    struct shadow_pair *pair_ptr = find_hash_pair(env, guest_eip);
    pair_ptr->host_eip = host_eip;
}

/*
 * helper_shack_flush()
 *  Reset shadow stack.
 */
void helper_shack_flush(CPUState *env)
{
    env->shack_top = env->shack;
}

/*
 * push_shack()
 *  Push next guest eip into shadow stack.
 */
void push_shack(CPUState *env, TCGv_ptr cpu_env, target_ulong next_eip) //next_eip contains the guest return address
{
    /* find the corresponding slot */
    struct shadow_pair *pair_ptr = find_hash_pair(env, next_eip);

    /* declare variables */
    TCGv_ptr tcg_shack_top = tcg_temp_new_ptr();
    TCGv_ptr tcg_shack_end = tcg_temp_new_ptr();
    int lab_push = gen_new_label();

    /* check if need flush
     * if(env->shack_top == env->shack_end)
     *      env->shack_top = env->shack;
     */
    tcg_gen_ld_ptr(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); //load shack top
    tcg_gen_ld_ptr(tcg_shack_end, cpu_env, offsetof(CPUState, shack_end)); //load shack end
    tcg_gen_brcond_i32(TCG_COND_NE, tcg_shack_top, tcg_shack_end, lab_push); //if(env->shack_top != env->shack_end): jmp to lab_push
    tcg_gen_ld_ptr(tcg_shack_top, cpu_env, offsetof(CPUState, shack)); //env->shack_top = env->shack (flush)
    tcg_gen_st_tl(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); // store shack top
    
    /* push the slot addr onto shack
     * env->shack_top = pair_ptr;
     * env->shack_top++;
     */
    gen_set_label(lab_push);
    tcg_gen_ld_ptr(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); //load shack top
    tcg_gen_st_tl(tcg_const_ptr(pair_ptr), tcg_shack_top, 0); // env->shack_top = pair_ptr
    tcg_gen_addi_ptr(tcg_shack_top, tcg_shack_top, sizeof (void *)); //shack_top++
    tcg_gen_st_tl(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); // store shack top

    /* clean up */
    tcg_temp_free_ptr(tcg_shack_top);
    tcg_temp_free_ptr(tcg_shack_end);
}

/*
 * pop_shack()
 *  Pop next host eip from shadow stack.
 */
void pop_shack(TCGv_ptr cpu_env, TCGv next_eip)
{
    /* declare variables */
    int lab_end = gen_new_label();
    TCGv tcg_next_eip = tcg_temp_local_new();
    TCGv_ptr tcg_top_host_eip = tcg_temp_local_new_ptr();

    TCGv_ptr tcg_shack_top = tcg_temp_new_ptr();
    TCGv_ptr tcg_shack = tcg_temp_new_ptr();
    TCGv tcg_top_guest_eip = tcg_temp_new();
    
    /* check if shack is empty */
    tcg_gen_mov_tl(tcg_next_eip, next_eip);
    tcg_gen_ld_ptr(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); //load shack top
    tcg_gen_ld_ptr(tcg_shack, cpu_env, offsetof(CPUState, shack)); //load shack
    tcg_gen_brcond_i32(TCG_COND_EQ, tcg_shack_top, tcg_shack, lab_end); // if(shack_top == shack): jmp to lab_end
    
    /* shack not empty -> update shack top*/
    tcg_gen_ld_ptr(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); //load shack top
    tcg_gen_subi_i32(tcg_shack_top, tcg_shack_top, sizeof (void *)); //else: shack_top--
    tcg_gen_st_tl(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); // store shack top

    /* check if shack_top->guest_eip == tcg_next_eip */
    tcg_gen_ld_ptr(tcg_shack_top, tcg_shack_top, 0); // shack top = shack[shack_top]
    tcg_gen_ld_tl(tcg_top_guest_eip, tcg_shack_top, offsetof(struct shadow_pair, guest_eip)); //tcg_top_guest_eip = tcg_shack_top->guest_eip    
    tcg_gen_brcond_i32(TCG_COND_NE, tcg_top_guest_eip, tcg_next_eip, lab_end); // if(shack_top->guest_eip != next_eip): jmp to lab_end

    /* check if shack_top->host_eip is valid */
    
    tcg_gen_ld_ptr(tcg_shack_top, cpu_env, offsetof(CPUState, shack_top)); //load shack top
    tcg_gen_ld_ptr(tcg_shack_top, tcg_shack_top, 0); // shack top = shack[shack_top]
    tcg_gen_ld_ptr(tcg_top_host_eip, tcg_shack_top, offsetof(struct shadow_pair, host_eip)); //tcg_top_host_eip = tcg_shack_top->host_eip
    tcg_gen_brcond_i32(TCG_COND_EQ, tcg_top_host_eip, tcg_const_ptr(NULL), lab_end); // if(shack_top->host_eip == NULL): jmp to lab_end

    /* update return addr*/
    *gen_opc_ptr++ = INDEX_op_jmp;
    *gen_opparam_ptr++ = tcg_top_host_eip;

    /* clean up */
    gen_set_label(lab_end);
    tcg_temp_free_ptr(tcg_shack_top);
    tcg_temp_free_ptr(tcg_shack);
    tcg_temp_free_ptr(tcg_top_host_eip);
    tcg_temp_free(tcg_top_guest_eip);
    tcg_temp_free(tcg_next_eip);
}

/*
 * Indirect Branch Target Cache
 */
__thread int update_ibtc;
struct ibtc_table *ibtc;
target_ulong saved_eip;

/*
 * helper_lookup_ibtc()
 *  Look up IBTC. Return next host eip if cache hit or
 *  back-to-dispatcher stub address if cache miss.
 */
void *helper_lookup_ibtc(target_ulong guest_eip)
{
    int key = guest_eip & IBTC_CACHE_MASK;
    if(ibtc->htable[key].guest_eip == guest_eip)
        return (void*)ibtc->htable[key].tb->tc_ptr;

    saved_eip = guest_eip;
    update_ibtc = 1;
    return optimization_ret_addr;
}

/*
 * update_ibtc_entry()
 *  Populate eip and tb pair in IBTC entry.
 */
void update_ibtc_entry(TranslationBlock *tb)
{
    int key = saved_eip & IBTC_CACHE_MASK;
    ibtc->htable[key].guest_eip = saved_eip;
    ibtc->htable[key].tb = tb;
    update_ibtc = 0;
}

/*
 * ibtc_init()
 *  Create and initialize indirect branch target cache.
 */
static inline void ibtc_init(CPUState *env)
{
    ibtc = (struct ibtc_table *) calloc(1, sizeof(struct ibtc_table));
    update_ibtc = 0;
}

/*
 * init_optimizations()
 *  Initialize optimization subsystem.
 */
int init_optimizations(CPUState *env)
{
    shack_init(env);
    ibtc_init(env);

    return 0;
}

/*
 * vim: ts=8 sts=4 sw=4 expandtab
 */
