/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <vka/vka.h>
#include <sel4/sel4.h>
#include <simple/simple.h>
#include <platsupport/irq.h>
#include <platsupport/io.h>

/*
 * Old struct, left here for compatibility reasons.
 */
typedef struct sel4ps_irq {
    cspacepath_t handler_path;
    cspacepath_t badged_ntfn_path;
    ps_irq_t irq;
} sel4ps_irq_t;

/* This is the maximum number of interrupts that can be bound on a particular
 * notification instance */
#define MAX_INTERRUPTS_TO_NOTIFICATIONS seL4_BadgeBits

typedef int ntfn_id_t;

/*
 * NOTE: This implementation of the platsuport IRQ interface is not thread-safe.
 */

/*
 * Initialises the IRQ interface.
 *
 * Requires a vka and simple as the interface will create IRQ handler caps and
 * also mint caps. The malloc interface that's passed in will be used to
 * allocate memory for the interface.
 *
 * @param irq_ops Interface to fill in
 * @param vka A VKA interface that must remain valid for the lifetime of the interface
 * @param simple A simple interface that must remain valid for the lifetime of the interface
 * @param max_irq_ids Maximum number of interrupts that can be registered
 * @param max_ntfn_ids Maximum number of notifications that can be provided
 * @param malloc_ops Malloc interface that is used to allocate memory for the IRQ interface
 *
 * @return 0 on success, otherwise an error code
 */
int sel4platsupport_new_irq_ops(ps_irq_ops_t *irq_ops, vka_t *vka, simple_t *simple,
                                size_t max_irq_ids, size_t max_ntfn_ids,
                                ps_malloc_ops_t *malloc_ops);

/*
 * The following functions are not intended to be used by drivers.
 *
 * They are intended to be used by the applications which wrap around the drivers.
 */

/*
 * Provides the IRQ interface with a notification object to be used for
 * receiving interrupts. A mask is to be provided to let the interface know
 * which bits of the badge can be used for interrupts. The notification object
 * can still be used by the user for other purposes.
 *
 * @param irq_ops Initialised IRQ interface
 * @param ntfn Notification cap that is to be provided
 * @param usable_mask Mask of bits that can be used for badging
 *
 * @return A valid notification ID with a value >= 0, otherwise an error code
 */
ntfn_id_t sel4platsupport_irq_provide_ntfn(ps_irq_ops_t *irq_ops, seL4_CPtr ntfn,
                                           seL4_Word usable_mask);

/*
 * This function is similar to `sel4platsupport_irq_provide_ntfn` except that
 * an notification ID hint can be provided. The interface will attempt to
 * allocate and provide an ID that is the same as the hint provided.
 *
 * @param irq_ops Initialised IRQ interface
 * @param ntfn Notification cap that is to be provided
 * @param usable_mask Mask of bits that can be used for badging
 * @param id_hint Hint specificying which ID is desired
 *
 * @return 0 on success, otherwise an error code
 */
int sel4platsupport_irq_provide_ntfn_with_id(ps_irq_ops_t *irq_ops, seL4_CPtr ntfn,
                                             seL4_Word usable_mask, ntfn_id_t id_hint);

/*
 * Returns the notification object back to the user. All interrupts that
 * are associated with the notification are all unbound. Also all minted
 * copies of the notification that the *IRQ interface* produces are deleted.
 *
 * @param irq_ops Initialised IRQ interface
 * @param ntfn_id ID of a notification that was provided to the interface
 * @param[out] ret_cptr Pointer to write the returned notification cap to, can be NULL
 *
 * @return 0 on success, otherwise an error code
 */
int sel4platsupport_irq_return_ntfn(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id,
                                    seL4_CPtr *ret_cptr);

/*
 * Pairs a registered interrupt with a registered notification object. Any
 * signals associated with the interrupt will arrive on the notification object
 * that was registered.
 *
 * @param irq_ops Initialised IRQ interface
 * @param ntfn_id ID of a notification that was provided to the interface
 * @param irq_id ID of a registered interrupt
 * @param[out] ret_badge Pointer to write the badge that was assigned to the notification when pairing, can be NULL
 *
 * @return 0 on success, otherwise an error code
 */
int sel4platsupport_irq_set_ntfn(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id,
                                 irq_id_t irq_id, seL4_Word *ret_badge);

/*
 * Unpairs a registered interrupt with its paired notification object. Pending
 * signals for the interrupt will simply be discarded.
 *
 * @param irq_ops Initialised IRQ interface
 * @param irq_id ID of a registered interrupt that is paired with a notification
 *
 * @return 0 on success, otherwise an error code
 */
int sel4platsupport_irq_unset_ntfn(ps_irq_ops_t *irq_ops, irq_id_t irq_id);

/*
 * Given a badge mask, this function will perform callbacks for all IRQs
 * registered on a notification that have matching badge bits.
 *
 * This function is mainly useful for the use case where the user reuses the
 * notification that was provided to the interface and waits/polls on it
 * without going through the interface.
 *
 * @param irq_ops Initialised IRQ interface
 * @param ntfn_id ID of a notification that was provided to the interface
 * @param handle_mask Badge mask of bits to check and perform callbacks for
 *
 * @return 0 on success, otherwise an error code
 */
int sel4platsupport_irq_handle(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id, seL4_Word handle_mask);

/*
 * Waits on a registered notification.
 *
 * If interrupts arrive, the interface will perform callbacks for those that
 * have matching bits in the mask.
 *
 * @param irq_ops Initialised IRQ interface
 * @param ntfn_id ID of a notification that was provided to the interface
 * @param wait_mask Badge mask of bits to perform callbacks for, if any and if there's a match
 * @param[out] ret_leftover_bits Pointer to write the portions of the badge that were not handled, can be NULL
 *
 * @return 0 on success, otherwise an error code
 */
int sel4platsupport_irq_wait(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id,
                             seL4_Word wait_mask, seL4_Word *ret_leftover_bits);

/*
 * This function follows the same functionality as `sel4platsupport_irq_wait`
 * except that it will not block.
 *
 * @param irq_ops Initialised IRQ interface
 * @param ntfn_id ID of a notification that was provided to the interface
 * @param poll_mask Badge mask of bits to perform callback for, if any and if there's a match
 * @param[out] ret_leftover_bits Pointer to write the portions of the badge that were not handled, can be NULL
 *
 * @returns 0 on success, otherwise an error code
 */
int sel4platsupport_irq_poll(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id,
                             seL4_Word poll_mask, seL4_Word *ret_leftover_bits);
