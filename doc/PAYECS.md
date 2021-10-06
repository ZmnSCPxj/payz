Payments Entity Component System
================================

This project includes an Entity Component System framework, as well as
built-in Systems for that framework that are used to implement a
payment attempt flow.

This document is an introduction to Entity Component Systems and the
specific framework used in this project.
For more information on how to extend the Payment ECS for your own
payment protocols, see [PAYECS-REF.md](PAYECS-REF.md).

Entity Component System
-----------------------

An Entity Component System framework is about the interaction of three
concepts:

* Entities - objects that you want to model.
* Components - parts of various objects; actual Entities are just a
  collection of their Components.
* Systems - code that operates on Entities with particular Components.

In the Payments ECS, entire payment commands are represented by an
Entity containing Components appropriate for tracking the status of the
payment command.
However, in addition, payment planning attempts (i.e. figuring out how
to make path(s) to the destination) are also represented by Entities.
Every (sub-)payment attempt is also represented by Entities.

The exact "type" of an Entity is defined by the Components it has; if
it has Components appropriate for a payment command, then it is a
payment command Entity.

Typically, an Entity is really nothing more than an integer identifier.
A Component will have a name and a data structure associated with the
name.

Both Entities and Components have no code; this is unlike typical
object-oriented paradigms.
Instead, (almost) all code is placed in Systems.

Systems operate on particular Components; if an Entity does not have
some Component that a System requires, then the System does not run
on that Component.
Many ECS frameworks will also allow Systems to specify disallowed
Components; if a disallowed Component exists on an Entity that would
otherwise be processed by a particular System, then the System skips
that Entity.

Entity-Component Table
----------------------

It helps to visualize the Entity and Components as being in a giant
spreadsheet or table.

Each row in a typical spreadsheet has a numeric label or ID; in
typical ECS frameworks this is the Entity ID.
Each column in a typical spreadsheet will often have a label in the
header row(s), and that label denotes what information is in that
cell in the column below it; in an ECS that is the Component name
and what the data type expected in the cells in that column are the
data structure of the Component.

Some cells in a spreadsheet may be empty; the Entity does not have
the corresponding Component (i.e. the Component is "detached" from
that Entity).
Others may have some value (i.e. the Component is "attahced" to the
Entity).

JSON Data And C-Lightning Notifications
---------------------------------------

In the Payments Entity Component System, all Component data must be
some JSON datum.
When we want to express a "detached" Component, we write it as the
special JSON datum `null`, i.e. `null` indicates that the Component
is not attached to some Entity (or, for the case of
`payecs_setcomponents`, that you want to *set* the Component to a
detached state or "detach the Component from the Entity").

By using JSON data, we allow plugins that want to use the Payment
ECS of this project to be written in any language that has *any*
library for JSON support, which is needed anyway since we use
JSON-RPC for calls between plugins and the main C-Lightning daemon.
That is: JSON is a *lingua franca* that allows diverse plugins to
work with the same framework.

Now of course if you want to modify the payment flow, you also have
to write *some* code to do whatever modifications it was that you
wanted.
In an ECS framework, that means you have to write some Systems and
somehow pass them to the ECS framework so that you can make a
modified payment flow.

This project then invokes Systems by broadcasting a C-Lightning
plugin notification named `payecs_system_invoke`.
Your plugin simply has to listen for this notification, and if its
`system` parameter matches the name of a System you registered, then
you call into the appropriate System code (you are encouraged to
write multiple small Systems instead of many larger ones, for
easier tracing of the payment flow later during debugging cycles,
so in practice you should write multiple small Systems).

So, in the Payment ECS:

* Entities are non-0 unsigned 32-bit numbers.
* Components have JSON string labels (should not have escape
  characters or control characters) and contain JSON datums.
* Systems are registered via the `payecs_newsystem` command and are
  invoked via the `payecs_system_invoke` notification.
  Plugins should listen to this notification to learn if their Systems
  are being invoked.

Payment ECS Notifications, Commands, and Special Components
===========================================================

Now that you have a basic understanding of the Payment ECS, let us
introduce the details.

`payecs_system_invoke` Notification
-----------------------------------

A notification for topic `payecs_system_invoke` is sent every time
an Entity is `payecs_advance`d and a matching System is found.

The parameters of this notification is this object:

```json
{
  "payload": {
      "system": "example:name_of_system",
      "entity": {
         "entity": 42,
         "example:required_component": {
           "some_structure": 100
         }
      }
  }
}
```

The `payload.system` field is the name of the matching System.
The Entity ID is found in the `payload.entity.entity` field,
and any required components indicated by the system will also be
added in the `payload.entity` object.

Plugins that want to register their own Systems have to listen
for this notification and check the `payload.system` field for
any of their registered Systems.
If any of their Systems match, then the plugin should call its
corresponding System code and pass it the entity and its
components.

`payecs_newsystem` Command
--------------------------

    payecs_newsystem system required [disallowed]

The **`payecs_newsystem`** RPC command informs the Payment ECS
Framework of a new System provided by a plugin.
The System name is given as a string in *`system`*.
This is a plain string, so as a convention you should prefix it
with your plugin or project name; for example, this project
prefixes all its systems with "`lightningd:`".

*`required`* is a non-empty array of strings, naming the Components
that must be present in an Entity in order for the registered
System to be invoked on that Entity.

*`disallowed`* is an optional array of strings (default empty
array), naming the Components that must *not* be present in an
Entity in order for athe registered System to be invoked on that
Entity.

All *`required`* Components must be attached on the Entity for
the given System to be invoked, and the `payecs_system_invoke`
notification will also include the values of those Components as
a convenience.
If *any* of the *`disallowed`* Components are on the Entity, then
the System will not operate on the Entity.

A System *should* detach a *`required`* Component or attach a
*`disallowed`* Component before running **`payecs_advance`** on
the Entity it is working on to continue processing; otherwise, it
may get triggered again in an infinite loop.

The **`payecs_newsystem`** RPC command is idempotent: if it
succeeded with a particular set of parameters, and it is called
again later with the exact same set of parameters (including
ordering of the *`required`* and *`disallowed`* arguments), the
second call will silently do nothing and succeed.

`lightningd:systems` Special Component
--------------------------------------

Every active Entity should have a `lightningd:systems` Component
attached to it.

The attached datum must be an object, and it must have the
field below:

* `systems` - An array of strings, naming the Systems that are
  candidates for triggering on this Entity.

The `payecs_advance` command checks for the object and scans
through the given `systems` array, searching for registered
Systems that match the Entity (i.e. have all their `required`
Components and none of their `disallowed` Components).

The `payecs_advance` command fills in these fields of the
`lightningd:systems` Component:

* `current` - a 0-based numeric index of the `systems` array,
  denoting the index of the system that was most recently
  matched.
* `error` - some JSONRPC error object in the case that the
  `payecs_advance` command cannot advance the payment:
  * The `lightningd:systems` Component is not attached or is
    not an object or has no `systems` field or the `systems`
    field is not an array of strings.
  * One of the listed `systems` is not registered and is not
    built-in.
  * None of the listed `systems` matched the current state
    of the Entity.

The Payment ECS includes many builtin systems which define a
particular default payment flow.
You can create a modified version of this payment flow by
using the `payecs_setdefaultsystems` command and passing in
your own Systems to `prepend` to the default flow.

Alternately, if you are willing or angry enough to reimplement
the entire payment flow yourself, or want to (ab)use the
*Payment* ECS for other purposes, you can simply attach an
arbitrary list of `systems` with whatever Systems you wish.

The default payment flow will often create Entities that
represent part of the "main" payment flow; the default payment
flow will copy the `lightningd:systems` `systems` field to the
sub-Entities.
If your Systems want to use sub-Entities yourself, then you
shuold similarly copy the `systems` field.

Note that `lightningd:systems` is otherwise "just another"
Component, and can be inspected with `payecs_getcomponents`
or `payecs_listentities` or mutated with `payecs_setcomponents`.

`payecs_advance` Command
------------------------

    payecs_advance entity

The **`payecs_advance`** RPC command attempts to continue the
processing for the given *`entity`*, the numeric ID of the
Entity to process.

Typically, when you want to start a new payment flow, you
get a fresh Entity ID via **`payecs_newentity`**, fill in all
the required Components on the returned Entity, then use
**`payecs_setdefaultsystems`** to set up the Systems (most
likely also passing in your own custom Systems) and then
finally **`payecs_advance`** the Entity.
Then you should continuously poll the state of the payment
Entity to determine its status (or for the default payment
flow, listen for the `pay_success` or `pay_failure`
notifications).

Also, each System you implement will process some data on
the Entity, and then once its own purpose has been finished,
it should typically **`payecs_advance`** the Entity it was
passed with so that other Systems can operate on the Entity.

If you want to terminate the processing of an Entity, you
should detach its `lightningd:systems` Component and not
call **`payecs_advance`**.

**`payecs_advance`** inspects the `lightningd:systems`
Component and modifies it before invoking the next
System.
If it is unable to find the next System to trigger, it
will fill in the `error` field of the `lightningd:systems`
Component before returning with an error.
Typical System code you provide should ignore errors from the
**`payecs_advance`** command and let the monitoring command
handle those.

This command returns an empty object.

`payecs_newentity` Command
--------------------------

    payecs_newentity

The **`payecs_newentity`** RPC command returns a new Entity ID
that you can use for your own purposes.

This is simply a counter that starts at 1 and keeps
incrementing by one at each call.

It returns the object:

```json
{
   "entity": 1
}
```

*`entity`* is the new Entity ID you should use for the new
Entity.

`payecs_getcomponents` Command
------------------------------

    payecs_getcomponents entity components

The **`payecs_getcomponents`** RPC command reads the current state
of the Components of the given Entity ID.

*`entity`* is the Entity ID of the Entity to read.

*`components`* is an array of strings, naming the Components of the
Entity to read.
As a convenience, it can also be a plain string, which implies that
that is the only Component to be read.

This individual command is "atomic" in that if multiple Components
are given, they will all be read in an atomic operation and with
no partial writes from parallel callers of the
**`payecs_setcomponents`** command.
However, if you need to ensure atomicity of a read-modify-write
operation, see the *`expected`* parameter of the
**`payecs_setcomponents`** command.

The command returns an object containing the entity ID and the
*`components`* you specify.
For example, if you passed in `["example:component"]` as the
*`components`* parameter:

```json
{
  "entity": 1,
  "example:component": 42
}
```

If the Entity is not attached to one or more of the Components
specified, then the corresponding field will be set to `null`.

`payecs_setcomponents` Command
------------------------------

    payecs_setcomponents writes [expected]

The **`payecs_setcomponents`** RPC command writes the specified
Entities, attaching or detaching Compponents according to the
*`writes`* specification, but if and only if all the *`expected`*
specifications match the current state.

*`writes`* is an array of objects; as a convenience, it can be an
object (which is treated as a single-entry array containing that
object).

Each object in *`writes`* should have an `entity` field, a numeric
Entity ID specifying the Entity to modify and how to modify it.
It may have an `exact` field; if the field is specified, it must be
the JSON `true`, and this means that before actually writing, all
Components of the Entity will be detached first.
All other fields will have the key naming a Component to mutate, and
the value being the value to write for that Component.
A `null` value means the Component will be detached, a non-`null`
value means the Component will be attached.

The optional *`expected`* is an array of objects; as a convenience,
it an be an object (which is treated as a single-entry array).

Each object in *`expected`* should have an `entity` field, a
numeric Entity ID specifying the Entity to validate.
It may have an `exact` field, indicating that the Entity should
not have any other attached Components that are not specified as
non-`null` in the specification object.
All other fields will have the key naming a Component to validate,
and the value being the expected value in that Component.
Comparison of the value is done "deeply", objects ignore the order
of keys (as is expected of JSON semantics).
A `null` value means the Component should be detached, a non-`null`
value means the Component must be attached and be deeply equal to
the value.

*Before* attempting to write, the command first atomically validates
that all *`expected`* specifications match.
If any of the *`expected`* specifications does not match the current
state of the ECS data table, then the command fails with error code
2244 without performing any *`writes`*.
If all of the *`expected`* specifications match, then the command
atomically performs all *`writes`* specified.

*`expected`* is used to ensure atomicity of a read-modify-write
sequence: if some parallel code mutates the ECS data in between you
reading the state with **`payecs_getcomponents`** and your write with
**`payecs_setcomponents`**, you can use *`expected`* to ensure to
catch this corner case and re-attempt the read and computation.
If you get the 2244 error code from this command, you should reread
all the inputs and recompute, then attempt the *`writes`* again
with the new inputs as *`expected`*.

In typical System code, if you want to mutate the Entity that was
passed in based *only* on the Components of that Entity, then
you do not need to use the *`expected`* field to ensure atomicity;
you will have exclusive "locked" access to the Entity until you
**`payecs_advance`** the Entity, which will then pass the implicit
lock to the next System (i.e. do not mutate the Entity after you
call **`payecs_advance`** on it).
However, if you are going to read or mutate other Entities that
might be mutated by Systems operating on other Entities, you
definitely need to ensure atomicity of read-modify-write operations
via this mechanism.

Because it does an atomic compare, you should be aware of ABA
Problems, and design your Components such that ABA Problems are
either not problematic or do not exist (e.g. by using a monotonic
counter).

This command returns an empty object.

`payecs_listentities` Command
-----------------------------

    payecs_listentities [required] [disallowed]

The **`payecs_listentities`** RPC command lists all Entities in
the Entity-Component table, with optional filtering based on
*`required`* or *`disallowed`* parameters.

If neither *`required`* nor *`disallowed`* are specified, then
this simply lists all Entities and the attached Components they
have.

If *`required`* is specified, then only Entities with all
*`required`* Components attached will be returned.
If *`disallowed`* is specified, if any of the *`disallowed`*
Components are attached, then the Entity will not be
returned.

It returns the object:

```json
{
  "entities": [
    {
      "entity": 1,
      "example:component: 42
    },
    {
      "entity": 42,
      "lightningd:systems": {
        "systems": ["example:system"]
      }
    }
  ]
}
```

`payecs_systrace` Command
-------------------------

    payecs_systrace entity

The **`payecs_systrace`** RPc command returns a trace of the
Systems that were triggered on the given *`entity`*, which
must be a numeric Entity ID.

Only the most recent 100,000 System invocations of all Entities
are retained, so old payment Entities may not have any trace data.
This command is intended for debugging of payment flows, so you
should not rely on its output for actual computation, as the
given limit may change at any new version.

It returns the object:

```json
{
  "entity": 1,
  "trace": [
    {
      "time": "2021-10-05T12:00:01.001Z",
      "system": "example:system_1",
      "entity": {
        "entity": 1,
        "example:component_1": "value"
      }
    },
    {
      "time": "2021-10-05T12:00:01.002Z",
      "system": "example:system_2",
      "entity": {
        "entity": 1,
        "example:component_2": 42
      }
    }
  ]
}
```

`payecs_setdefaultsystems` Command
----------------------------------

    payecs_setdefaultsystems entity [prepend] [append]

The **`payecs_setdefaultsystems`** RPC command sets up the given
*`entity`* for the normal payment flow Systems; you may modify
the default payment flow by adding your own systems in the
*`prepend`* or *`append`* parameters.

*`entity`* is a numeric Enttiy ID specifying the Entity to set up.

*`prepend`* is an array of strings naming Systems that will be
prepended to the list of default Systems.
Since they are prepended, they will match first, and can be used to
override existing Systems.

*`append`* is an array of strings naming Systems that will be
appended to the list of default Systems.
Since they are appended, default Systems will match first, and can
be used to handle cases that are ignored by the default Systems.

It returns an empty object.

`payecs_getdefaultsystems` Command
----------------------------------

    payecs_getdefaultsystems

the **`payecs_getdefaultsystems`** RPC command returns the list of
built-in Systems that implement the default payment flow.

It returns the object:

```json
{
  "systems": [
    "lightningd:example_builtin_system_1",
    "lightningd:example_builtin_system_2",
    "lightningd:example_builtin_system_n"
  ]
}
```

The returned object can be set as the `lightningd:systems` Component.
