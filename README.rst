wikidice
--------

What does this do?
==================

Wikipedia has its own built-in "Random Page" function. You click it and it gives
you a random page. But what if you want to learn about a random topic within a
specific category? For example, I want to learn about a random topic within the
"Computer Science" category. There's no way to do that with Wikipedia's built-in
"Random Page" function. This is what `wikidice` is for. It serves a random
Wikipedia article within a specific category--recursively, including any page
within any of the nested subcategories.

The data comes from the `Wikipedia database dumps
<https://dumps.wikimedia.org/>`_. These are SQL dump files distributed for free
by Wikipedia. Specifically, we need the dumps for the ``categorylinks`` table, the
``page`` table, and the ``category`` table.

This used to be a Go application that communicated with a MySQL database. The
problem with this was, it was very costly to import the SQL dumps into MySQL. It
would take several days to import the SQL dumps, then maybe a day or two more to
populate the tables in a fashion which is efficient for these kinds of random
lookups. Also, the MySQL database was huge, requiring 50GB+ of disk space on a
server, which didn't make sense for a simple toy-ish application like this which
makes no money. This latest version is a rewrite where the data is prepared by a
C++ program. This C++ program parses the SQL dumps and stores it in an
efficient, compressed RocksDB database. To build the database using this C++
program, you need beefy hardware. It needs at least 8GB of RAM and 100GB of
disk--preferably NVME or a ramdisk even. The program reads the SQL dumps and
processes them in parallel using all available cores. On an AMD Ryzen 9 with 32
cores, it takes ~2-5 hours to build the database. However, despite the high
initial cost of building the database (at least in terms of processing power),
the resulting database is only ~1.5GB in size. This is a huge improvement over
the MySQL database, which was 50GB+. 1.5GB is reasonable to host on a cheap ec2
or VPS.

A user-friendly Web frontend is forthcoming, but for now an HTTP API is built
and implemented.

Installation
============

Dependencies:

- C++20 compiler (GCC 10+ or Clang 10+)
- CMake 3.24+
- Zstandard 1.5+
- Python3.12+ including Python development headers

All other dependencies, including RocksDB, are vendored in ``/external`` and built with the CMake project.

To build: ::

.. code-block:: bash
  $ git clone https://github.com/proprietary/wikidice.git
  $ cd wikidice
  $ mkdir build
  $ cd build
  $ cmake -DCMAKE_BUILD_TYPE=Release ..
  $ make
  $ cd ..
  $ ./create-database.sh


Disclaimer
==========

Not associated or affiliated with Wikipedia in any way.

License
=======

Apache-2.0
