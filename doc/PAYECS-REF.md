Payments ECS Components Reference
=================================

Interacting with the default payment flow is done via Components in the
Entity Component System framework.

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
you want to continue processing, then you should have a System do
this check and detach the Component to prevent the error, and possibly
modify your payment flow (by attaching your own Components) so you can
somehow pay using the destination currency.

The default flow will promote the `lightningd:invoice:type` Component
value to another attached component.
For example, if the `lightningd:invoice:type` Component is `"bolt11
invoice"`, then a Component named `lightningd:invoice:type:bolt11 invoice`
will be attached to the value `true`.
Your own Systems can trigger on the promoted Component name.

### `lightningd:nonce`

A random 256-bit number attached by the default flow to all Entities.
This is a 64-character hex string.

This should not be used as a private key, as the nonce is not protected in
some process with restricted inputs and reduced permissions, and may be more
easily exfiltrated.
It is also not necessarily cryptographically strong randomness.
It is appropriate for use as a seed for route randomization or shadow
routing, however.
