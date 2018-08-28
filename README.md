# Differential Accessor to History Log

## Purpose

Proof of concept and test a reliable way to transfer values out an history with a limited capacity (meaning oldest events are destroyed by newest events when history is full)

By transfer it must be understood, we can first get all the available history entries (from oldest to newest)

As the history can have new elements added between transfers, subsequent transfers will only get the added element (still in the old to new order)

If the number of added elements exceed the history capacity during a transfer, oldest elements will be lost

This type of transfer is done through a so called *differential accessor*

## API for the Differential Accessor  Transfer

`initDifferentialAccessor()`

- restarts the transfer considering all elements in the history as not already transferred

`getFromDifferentialAccessor(historyItem* pValue)`

- get next element (from oldest) if any available
- if so, returns a *non null* value and affects `*pValue` with history item
- if nothing available (everything already transferred or history empty), returns *null* and does not alter anything in `*pValue`

## Suggested Compilation

`gcc -o0 -ggdb -Wall test.c -o test`

## Proof of Concept

In this proof of concept, history is called a 'bucket'

## Unit Tests

Unit tests are implemented to try out different type of scenarii:

   - when bucket is full or not
   - when it is flushed
   - when events are added while read
   - when events are lost because of a full unread bucket