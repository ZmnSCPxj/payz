Payments ECS Components Reference
=================================

Interacting with the default payment flow is done via Components in the
Entity Component System framework.

If you need to modify the payment flow, you should:

* Create a new Entity and set up its Components for a "main" payment
  Entity, including `lightningd:main-payment`, as well as other Components
  such as `lightningd:invoice` or `lightningd:amount`.
* Set up the default flow with your own modifying Systems, by passing in
  your Systems to `payecs_setdefaultsystems`.
* `payecs_advance` the Entity.
* Wait for the Entity processing to complete.

This document is a reference to the Components exposed by the default
payment flow.
You can modify the payment flow by adding your own Systems which trigger
on relevant Components, and attaching particular Components to prevent
parts of the default behavior from triggering.

Thus, this reference contains necessary information on how to extend the
Payment ECS default flow for your own payment protocols.

The actual default Systems will use more Components than are listed in
this reference, but undocumented Components should be considered as
subject to change at any future version of this project.
All Components used by this project have the prefix `lightningd:`; you
should avoid this in your own plugin, and we recommend using your own
plugin or project name as a prefix for your Components.

For more information on the basics of the Payment ECS, see
[PAYECS.md](PAYECS.md).

Components
----------

### `lightningd:amount`

A string containing the millisatoshi amount to pay to the destination,
with the unit `msat` appended to it.

The default payment flow requires this Component so it knows how much to
deliver to the destination, but will copy it from a
`lightningd:invoice:amount_msat` Component if one is attached (and will fail
the Entity if both are attached).

### `lightningd:error`

A JSON-RPC 2.0 Error Object.

The default payment flow attaches this Component to an Entity in case of
errors.
In that case, it will detach `lightningd:systems`, indicating that
processing of the Entity has aborted.

If the main payment Entity you created gains this Component, you should
report the error as well to the caller of your plugin.

### `lightningd:exemptfee`

A string containing a millisatoshi amount with `msat` appended to it.
This is some minimum amount of fee that we will budget.
This should be attached to the main payment Entity.

The default flow will attach a reasonable default value to this Component
on a main payment entity if it is not attached.

See also `lightningd:feebudget`.

### `lightningd:feebudget`

A string containing a millisatoshi amount with `msat` appended to it.
This is the maximum amount of fees that payment planning will allocate.
This should be attached to the main payment Entity.

If the following Components are attached:

* `lightningd:amount`
* `lightningd:maxfeepercent`
* `lightningd:exemptfee`

...*and* `lightningd:feebudget` is *not* attached, then the default flow
will compute the `lightningd:feebudget` from the above Components.
(to be implemented)
Specifically:

* It will compute a fee budget by multiplying `lightningd:amount` by the
  `lightningd:maxfeepercent`.
* If the result is greater than `lightningd:exemptfee`, use the above
  product.
* Otherwise use `lightningd:exemptfee`.

The default payment flow will use this to provide a budget to payment
planning.

### `lightningd:invoice` and Family

A string that should be attached when you initialize a main payment
Entity.
This is a BOLT11 invoice string.

The default flow will parse the `lightningd:invoice` string using the
**`decode`** command.
If it parses as valid, then the fields returned by the **`decode`**
command are prepended with `lightningd:invoice:` and attached as
components of the Entity.

For example, if the string in `lightningd:invoice` is a BOLT11 invoice
specifying an amount, then `lightningd:invoice:amount_msat` is set to
the amount parsed.

The default flow will fail the Entity and stop processing if the
`lightningd:invoice:currency` does not match your node currency; if
you want to continue processing, then you should have a prepended System
do this check and detach the Component to prevent the error, and possibly
modify your payment flow (by attaching your own Components) so you can
somehow pay using the destination currency.
(to be implemented)

The default flow will promote the `lightningd:invoice:type` Component
value to another attached component.
For example, if the `lightningd:invoice:type` Component is `"bolt11
invoice"`, then a Component named `lightningd:invoice:type:bolt11 invoice`
will be attached to the value `true`.
Your own Systems can trigger on the promoted Component name.

The default flow will check if `lightningd:amount` does not exist but
`lightningd:invoice:amount_msat` does; if so, it detaches the
`lightningd:invoice:amount_msat` and attaches its value to
`lightningd:amount`.
If the default flow finds both, it will fail entity processing with an
error in `lightningd:error`.

### `lightningd:main-payment`

Marks the entity as being the "main" Entity representing an entire
payment.

The default flow ignores the actual value of this Component.

When setting up an Entity using your modified version of the default
flow, make sure to also attach this Component.
You can then rely on this Component to trigger any Systems you add
that must run only on the main payment Entity.

### `lightningd:maxdelay`

A positive integer, unit of blocks, indicating the maximum number of
blocks that we will allow a payment path to resolve.
This should be attached to the main payment Entity.

The default flow will attach a reasonable default value to this Component
on a main payment entity if it is not attached.

The default flow, when creating payment planning Entities, will copy this
Component to the payment planning Entity.

### `lightningd:maxfeepercent`

A floating-point number, unit of percentage, indicating how much of the
`lightningd:amount` to allocate to fees.
This should be attached to the main payment Entity.

The default flow will attach a reasonable default value to this Component
on a main payment entity if it is not attached.

See also `lightningd:feebudget`.

### `lightningd:nonce`

A random 256-bit number attached by the default flow to all Entities.
This is a 64-character hex string.

This should not be used as a private key, as the nonce is not protected in
some process with restricted inputs and reduced permissions, and may be more
easily exfiltrated.
It is also not necessarily cryptographically strong randomness.
It is appropriate for use as a seed for route randomization or shadow
routing, however.

### `lightningd:retry_for`

A number of seconds, indicating how long to keep retrying the payment.
This should be attached to the main payment Entity.

The default flow will attach a reasonable default value to this Component
on a main payment entity if it is not attached.

### `lightningd:riskfactor`

A number representing an expected percent of return on investment, used
in converting block-of-delay costs to msatoshi.
This should be attached to the main payment Entity.

The default flow will attach a reasonable default value to this Component
on a main payment entity if it is not attached.

The default flow, when creating payment planning Entities, will copy this
Component to the payment planning Entity.

