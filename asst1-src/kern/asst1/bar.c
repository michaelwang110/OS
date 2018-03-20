#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "bar.h"
#include "bar_driver.h"
#include "Queue.h"

/* Declare any globals you need here (e.g. locks, etc...) */
Queue pending_orders;
struct lock *que_lock;
struct cv *order_made;

/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_drink()
 *
 * Takes one argument referring to the order to be filled. The
 * function makes the order available to staff threads and then blocks
 * until a bartender has filled the glass with the appropriate drinks.
 */
void order_drink(struct barorder *order) {
	order->order_ready = cv_create("order ready");
	lock_acquire(que_lock);
	enqueue(pending_orders, order);
	lock_release(que_lock);
	cv_signal(order_made, que_lock);
	cv_destroy(order->order_ready);
}

/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY BARTENDER THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it returns a pointer to the order.
 *
 */
struct barorder *take_order(void) {
	struct barorder *ret = NULL;
	//loop - while true
	//wait on order_ready
	lock_acquire(que_lock);
	cv_wait(order_made, que_lock);
	//dequeue();
	lock_release(que_lock);
	return ret;
}

/*
 * fill_order()
 *
 * This function takes an order provided by take_order and fills the
 * order using the mix() function to mix the drink.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE
 * REQUIRED BOTTLES (AND, IDEALLY, ONLY THE BOTTLES) IT NEEDS TO USE TO
 * FILL THE ORDER.
 */
void fill_order(struct barorder *order) {

	/* add any sync primitives you need to ensure mutual exclusion
	holds as described */

	/* the call to mix must remain */
	mix(order);

}

/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct barorder *order) {
	(void) order; /* avoid a compiler warning, remove when you start */
}

/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */

/*
 * bar_open()
 *
 * Perform any initialisation you need prior to opening the bar to
 * bartenders and customers. Typically, allocation and initialisation of
 * synch primitive and variable.
 */
void bar_open(void) {
	pending_orders = create_queue();
	que_lock = lock_create("queue lock");
	order_made = cv_create("order made");
}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */
void bar_close(void) {
	dispose_queue(pending_orders);
	lock_destroy(que_lock);
	cv_destroy(order_made);
}
