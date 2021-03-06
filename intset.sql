-- CREATE TYPE intSet;

CREATE FUNCTION intset_in(cstring)
   RETURNS intset
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION intset_out(intset)
   RETURNS cstring
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset'
   LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE intSet (
   internallength = variable,
   input = intset_in,
   output = intset_out
);

-- define the required operators

CREATE FUNCTION intset_contains(int, intSet) RETURNS bool
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR ? (
   leftarg = integer, 
   rightarg = intSet, 
   procedure = intset_contains,
   commutator = ? , 
   negator = !?
);

CREATE FUNCTION get_cardinality(intSet) RETURNS int
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR # (
   rightarg = intSet, 
   procedure = get_cardinality
);

CREATE FUNCTION contains_all(intSet, intSet) RETURNS bool
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR >@ (
   leftarg = intSet,
   rightarg = intSet,
   procedure = contains_all,
   commutator = >@
);

CREATE FUNCTION contains_only(intSet, intSet) RETURNS bool
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR @< (
   leftarg = intSet,
   rightarg = intSet,
   procedure = contains_only,
   commutator = @<
);

CREATE FUNCTION equal(intSet, intSet) RETURNS bool
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR = (
   leftarg = intSet,
   rightarg = intSet,
   procedure = equal,
   commutator = =,
   negator = <>
);

CREATE FUNCTION not_equal(intSet, intSet) RETURNS bool
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <> (
   leftarg = intSet,
   rightarg = intSet,
   procedure = not_equal,
   commutator = <>,
   negator = =
);


CREATE FUNCTION intersection(intSet, intSet) RETURNS intSet
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR && (
   leftarg = intSet,
   rightarg = intSet,
   procedure = intersection,
   commutator = &&
);


CREATE FUNCTION union_set(intSet, intSet) RETURNS intSet
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR || (
   leftarg = intSet,
   rightarg = intSet,
   procedure = union_set,
   commutator = ||
);

CREATE FUNCTION disjunction(intSet, intSet) RETURNS intSet
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR !! (
   leftarg = intSet,
   rightarg = intSet,
   procedure = disjunction,
   commutator = !!
);

CREATE FUNCTION difference(intSet, intSet) RETURNS intSet
   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/intset' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR - (
   leftarg = intSet,
   rightarg = intSet,
   procedure = difference,
   commutator = -
);

-- CREATE FUNCTION intSet_

-- create the support function too
-- CREATE FUNCTION complex_abs_cmp(complex, complex) RETURNS int4
--   AS '/srvr/z5288155/postgresql-12.5/src/tutorial/complex' LANGUAGE C IMMUTABLE STRICT;

-- now we can make the operator class


-- now, we can define a btree index on intSet types. First, let's populate
-- the table. Note that postgres needs many more tuples to start using the
-- btree index during selects.

-- INSERT INTO DBSystems VALUES ('(56.0,-22.5)', '(-43.2,-0.07)');
-- INSERT INTO DBSystems VALUES ('(-91.9,33.6)', '(8.6,3.0)');

-- CREATE INDEX test_cplx_ind ON test_complex
--   USING btree(a complex_abs_ops);

-- SELECT * from test_complex where a = '(56.0,-22.5)';
-- SELECT * from test_complex where a < '(56.0,-22.5)';
-- SELECT * from test_complex where a > '(56.0,-22.5)';


-- clean up the example
-- DROP TABLE test_complex;
-- DROP TYPE complex CASCADE;
