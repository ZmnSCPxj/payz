PayZ
====

Plugin for an alternative to the normal C-Lightning
`pay` algorithm.

Goals
-----

* Allow other plugins, written in non-C languages, to
  utilize modified versions of the current payment flow
  by inserting their own code to the payment flow.
  * This will be done by using an Entity Component System
    framework, which will leverage the JSON interface to
    allow interoperation between the C code of PayZ and
    that of arbitrary C-Lightning plugins.
* Use mincostflow algorithms and various innovations by
  Richter and Pickhardt, hopefully for better payment
  success.
* The intent is that this project will eventually get
  merged into the actual C-Lightning, which is why it is
  written in C and uses extensive parts of C-Lightning as
  well.
