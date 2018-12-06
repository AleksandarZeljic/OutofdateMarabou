/*********************                                                        */
/*! \file Tableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __Tableau_h__
#define __Tableau_h__

#include "IBasisFactorization.h"
#include "ITableau.h"
#include "MString.h"
#include "Map.h"
#include "Set.h"
#include "SparseColumnsOfBasis.h"
#include "SparseMatrix.h"
#include "SparseTableauRow.h"
#include "SparseUnsortedList.h"
#include "Statistics.h"

class Equation;
class ICostFunctionManager;
class PiecewiseLinearCaseSplit;
class TableauState;

class Tableau : public ITableau, public IBasisFactorization::BasisColumnOracle
{
public:
    Tableau();
    ~Tableau();

    /*
      Allocate space for the various data structures
      n: total number of variables
      m: number of constraints (rows)
    */
    void setDimensions( unsigned m, unsigned n );

    /*
      Initialize the constraint matrix
    */
    void setConstraintMatrix( const double *A );

    /*
      Set which variable will enter the basis. The input is the
      index of the non basic variable, in the range [0, n-m).
    */
    void setEnteringVariableIndex( unsigned nonBasic );

    /*
      Set which variable will leave the basis. The input is the
      index of the basic variable, in the range [0, m).
    */
    void setLeavingVariableIndex( unsigned basic );

    /*
      Get the current set of basic variables
    */
    Set<unsigned> getBasicVariables() const;

    /*
      Set/get the values of the right hand side vector, b, of size m.
      Set either the whole vector or a specific entry
    */
    void setRightHandSide( const double *b );
    void setRightHandSide( unsigned index, double value );
    const double *getRightHandSide() const;

    /*
      Perform backward/forward transformations using the basis factorization.
    */
    void forwardTransformation( const double *y, double *x ) const;
    void backwardTransformation( const double *y, double *x ) const;

    /*
      Mark a variable as basic in the initial basis
     */
    void markAsBasic( unsigned variable );

    /*
      Initialize the tableau matrices (_B and _AN) according to the
      initial set of basic variables. Assign all non-basic variables
      to lower bounds and computes the assignment. Assign the initial basic
      indices according to the equations.
    */
    void initializeTableau( const List<unsigned> &initialBasicVariables );
    void assignIndexToBasicVariable( unsigned variable, unsigned index );

    /*
      A method for adding an additional equation to the tableau. The
      method returns the index of the fresh auxiliary variable assigned
      to the equation
    */
    unsigned addEquation( const Equation &equation );

    /*
      Get the Tableau's dimensions.
    */
    unsigned getM() const;
    unsigned getN() const;

    /*
      Get the assignment of a variable, either basic or non-basic
    */
    double getValue( unsigned variable );

    /*
      Given an index of a non-basic variable in the range [0,n-m),
      return the original variable that it corresponds to.
    */
    unsigned nonBasicIndexToVariable( unsigned index ) const;

    /*
      Given an index of a basic variable in the range [0,m),
      return the original variable that it corresponds to.
    */
    unsigned basicIndexToVariable( unsigned index ) const;

    /*
      Given a variable, returns the index of that variable. The result
      is in range [0,m) if the variable is basic, or in range [0,n-m)
      if the variable is non-basic.
    */
    unsigned variableToIndex( unsigned index ) const;

    /*
      Set the lower/upper bounds for a variable. These functions are
      meant to be used as part of the initialization of the tableau.
    */
    void setLowerBound( unsigned variable, double value );
    void setUpperBound( unsigned variable, double value );

    /*
      Get the lower/upper bounds for a variable.
    */
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;

    /*
      Get all lower and upper bounds.
    */
    const double *getLowerBounds() const;
    const double *getUpperBounds() const;

    /*
      Recomputes bound valid status for all variables.
    */
    void checkBoundsValid();

    /*
      Sets bound valid flag to false if bounds are invalid
      on the given variable.
    */
    void checkBoundsValid( unsigned variable );

    /*
      Returns whether any variable's bounds are invalid.
    */
    bool allBoundsValid() const;

    /*
      Tighten the lower/upper bound for a variable. These functions
      are meant to be used during the solution process, when a tighter
      bound has been discovered.
    */
    void tightenLowerBound( unsigned variable, double value );
    void tightenUpperBound( unsigned variable, double value );

    /*
      Return the current status of the basic variable
    */
    unsigned getBasicStatus( unsigned basic );
    unsigned getBasicStatusByIndex( unsigned basicIndex );

    /*
      True if there exists some out-of-bounds basic
    */
    bool existsBasicOutOfBounds() const;

    /*
      Compute the basic assignment from scratch.
    */
    void computeAssignment();

    /*
      Compute the status of the basic variable based on current assignment
    */
    void computeBasicStatus();
    void computeBasicStatus( unsigned basic );

    /*
      Picks the entering variable.
    */
    bool eligibleForEntry( unsigned nonBasic, const double *costFunction ) const;
    unsigned getEnteringVariable() const;
    unsigned getEnteringVariableIndex() const;
    bool nonBasicCanIncrease( unsigned nonBasic ) const;
    bool nonBasicCanDecrease( unsigned nonBasic ) const;

    /*
      Pick the leaving variable according to the entering variable.
      d is the column vector for the entering variable (length m)
    */
    void pickLeavingVariable();
    void pickLeavingVariable( double *d );
    unsigned getLeavingVariable() const;
    unsigned getLeavingVariableIndex() const;
    double getChangeRatio() const;
    void setChangeRatio( double changeRatio );

    /*
      Returns true iff the current iteration is a fake pivot, i.e. the
      entering variable jumping from one bound to the other.
    */
    bool performingFakePivot() const;

    /*
      Performs the pivot operation after the entering and leaving
      variables have been selected
    */
    void performPivot();

    /*
      Performs a degenerate pivot: just switches the entering and
      leaving variable. The leaving variable is required to be within
      bounds, so that is remains within bounds as a non-basic
      variable. Assignment values are unchanged (and the assignment is
      remains valid).
     */
    void performDegeneratePivot();

    /*
      Calculate the ratio constraint for the entering variable
      imposed by a basic variable.
      Coefficient is the relevant coefficient in the tableau.
      Decrease is true iff the entering variable is decreasing.
    */
    double ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease );

    /*
      True iff the variable is basic
    */
    bool isBasic( unsigned variable ) const;

    /*
      Set the assignment of a non-basic variable to a given value.
      If updateBasics is true, also update any basic variables that depend on this non-basic.
    */
    void setNonBasicAssignment( unsigned variable, double value, bool updateBasics );

    /*
      Compute the cost function (for all variables).
    */
    void computeCostFunction();

    /*
      Updat the cost function for an adjacement basis. Requires the
      change column and pivot row to have been computed previously.
    */
    void updateCostFunctionForPivot();

    /*
      Get a list of non-basic variables eligible for entry into the
      basis, i.e. variables that can be changed in a way that would
      reduce the cost value.
    */
    void getEntryCandidates( List<unsigned> &candidates ) const;

    /*
      Compute the multipliers for a given list of row coefficient.
    */
    void computeMultipliers( double *rowCoefficients );

    /*
      Access the cost function.
    */
    const double *getCostFunction() const;

    /*
      Compute the "change column" d, given by inv(B) * a. This is the column
      that tells us the change rates for the basic variables, when the entering
      variable changes.

      Can also get the column or manually set it.
    */
    void computeChangeColumn();
    const double *getChangeColumn() const;
    void setChangeColumn( const double *column );

    /*
      Compute the pivot row.
    */
    void computePivotRow();
    const TableauRow *getPivotRow() const;
    const SparseTableauRow *getSparsePivotRow() const;

    /*
      Dump the tableau (debug)
    */
    void dump() const;
    void dumpAssignment();
    void dumpEquations();

    /*
      Extract a row from the tableau.
    */
    void getTableauRow( unsigned index, TableauRow *row );
    void getTableauRow( unsigned index, SparseTableauRow *row );

    /*
      Get the original constraint matrix A or a column thereof,
      in dense form.
    */
    const SparseMatrix *getSparseA() const;
    const double *getAColumn( unsigned variable ) const;
    void getSparseAColumn( unsigned variable, SparseUnsortedList *result ) const;
    void getSparseARow( unsigned row, SparseUnsortedList *result ) const;
    const SparseUnsortedList *getSparseAColumn( unsigned variable ) const;
    const SparseUnsortedList *getSparseARow( unsigned row ) const;

    /*
      Store and restore the Tableau's state. Needed for case splitting
      and backtracking. The stored elements are the current:

      - Tableau dimensions
      - The current matrix A
      - Lower and upper bounds
      - Basic variables
      - Basic and non-basic assignments
      - The current indexing
      - The current basis
    */
    void storeState( TableauState &state ) const;
    void restoreState( const TableauState &state );

    /*
      Register or unregister to watch a variable.
    */
    void registerToWatchAllVariables( VariableWatcher *watcher );
    void registerToWatchVariable( VariableWatcher *watcher, unsigned variable );
    void unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable );

    /*
      Register to watch for tableau dimension changes.
    */
    void registerResizeWatcher( ResizeWatcher *watcher );

    /*
      Register the cost function manager.
    */
    void registerCostFunctionManager( ICostFunctionManager *costFunctionManager );

    /*
      Notify all watchers of the given variable of a value update,
      or of changes to its bounds.
    */
    void notifyVariableValue( unsigned variable, double value );
    void notifyLowerBound( unsigned variable, double bound );
    void notifyUpperBound( unsigned variable, double bound );

    /*
      Have the Tableau start reporting statistics.
     */
    void setStatistics( Statistics *statistics );

    /*
      Compute the current sum of infeasibilities
    */
    double getSumOfInfeasibilities() const;

    /*
      The current state and values of the basic assignment
    */
    BasicAssignmentStatus getBasicAssignmentStatus() const;
    double getBasicAssignment( unsigned basicIndex ) const;
    void setBasicAssignmentStatus( ITableau::BasicAssignmentStatus status );

    /*
      True if the basic variable is out of bounds
    */
    bool basicOutOfBounds( unsigned basic ) const;
    bool basicTooHigh( unsigned basic ) const;
    bool basicTooLow( unsigned basic ) const;

    /*
      Methods for accessing the basis matrix and extracting
      from it explicit equations. These operations may be
      costly if the explicit basis is not available - this
      also depends on the basis factorization in use.

      These equations correspond to: B * xB + An * xN = b

      Can also extract the inverse basis matrix.
    */
    bool basisMatrixAvailable() const;
    void makeBasisMatrixAvailable();
    double *getInverseBasisMatrix() const;

    void getColumnOfBasis( unsigned column, double *result ) const;
    void getColumnOfBasis( unsigned column, SparseUnsortedList *result ) const;
    void getSparseBasis( SparseColumnsOfBasis &basis ) const;

    /*
      Trigger a re-computing of the basis factorization. This can
      be triggered, e.g., if a high numerical degradation has
      been observed.
    */
    void refreshBasisFactorization();

    /*
      Merge two columns of the constraint matrix and re-initialize
      the tableau.
    */
    void mergeColumns( unsigned x1, unsigned x2 );

    /*
      When we start merging columns, variables effectively get renamed.
      Further, it is possible that x1 is renamed to x2, and x2 is then
      renamed to x3. This function returns the final name of its input
      variable, after the merging has been applied.
     */
    unsigned getVariableAfterMerging( unsigned variable ) const;

private:
    /*
      Variable watchers
    */
    typedef List<VariableWatcher *> VariableWatchers;
    HashMap<unsigned, VariableWatchers> _variableToWatchers;
    List<VariableWatcher *> _globalWatchers;

    /*
      Resize watchers
    */
    List<ResizeWatcher *> _resizeWatchers;

    /*
      The dimensions of matrix A
    */
    unsigned _n;
    unsigned _m;

    /*
      The constraint matrix A, and a collection of its
      sparse columns. The matrix is also stored in dense
      form (column-major).
    */
    SparseMatrix *_A;
    SparseUnsortedList **_sparseColumnsOfA;
    SparseUnsortedList **_sparseRowsOfA;
    double *_denseA;

    /*
      Used to compute inv(B)*a
    */
    double *_changeColumn;

    /*
      Used to store the pivot row
    */
    TableauRow *_pivotRow;
    SparseTableauRow *_sparsePivotRow;

    /*
      The right hand side vector of Ax = b
    */
    double *_b;

    /*
      Working memory (of size m and n).
    */
    double *_workM;
    double *_workN;

    /*
      A unit vector of size m
    */
    double *_unitVector;

    /*
      The current factorization of the basis
    */
    IBasisFactorization *_basisFactorization;

    /*
      The multiplier vector
    */
    double *_multipliers;

    /*
      Mapping between basic variables and indices (length m)
    */
    unsigned *_basicIndexToVariable;

    /*
      Mapping between non-basic variables and indices (length n - m)
    */
    unsigned *_nonBasicIndexToVariable;

    /*
      Mapping from variable to index, either basic or non-basic
    */
    unsigned *_variableToIndex;

    /*
      The set of current basic variables
    */
    Set<unsigned> _basicVariables;

    /*
      The assignment of the non basic variables.
    */
    double *_nonBasicAssignment;

    /*
      Upper and lower bounds for all variables
    */
    double *_lowerBounds;
    double *_upperBounds;

    /*
      Whether all variables have valid bounds (l <= u).
    */
    bool _boundsValid;

    /*
      The current assignment for the basic variables
    */
    double *_basicAssignment;

    /*
      The current status of the basic variabels
    */
    unsigned *_basicStatus;

    /*
      A non-basic variable chosen to become basic in this iteration
    */
    unsigned _enteringVariable;

    /*
      A basic variable chosen to become non-basic in this iteration
    */
    unsigned _leavingVariable;

    /*
      The amount by which the entering variable should change in this
      pivot step
    */
    double _changeRatio;

    /*
      True if the leaving variable increases, false otherwise
    */
    bool _leavingVariableIncreases;

    /*
      The status of the basic assignment
    */
    BasicAssignmentStatus _basicAssignmentStatus;

    /*
      Statistics collection
    */
    Statistics *_statistics;

    /*
      The cost function manager
    */
    ICostFunctionManager *_costFunctionManager;

    /*
      _mergedVariables[x] = y means that x = y, and that
      variable x has been merged into variable y. So, when
      extracting a solution for x, we should read the value of y.
     */
    Map<unsigned, unsigned> _mergedVariables;

    /*
      Free all allocated memory.
    */
    void freeMemoryIfNeeded();

    /*
      Resize the relevant data structures to add a new row to the tableau.
    */
    void addRow();

    /*
      Update the variable assignment to reflect a pivot operation,
      without re-computing it from scratch.
     */
    void updateAssignmentForPivot();

    /*
      Ratio tests for determining the leaving variable
    */
    void standardRatioTest( double *changeColumn );
    void harrisRatioTest( double *changeColumn );

    static void log( const String &message );

    /*
      For debugging purposes only
    */
    void verifyInvariants();

    static String basicStatusToString( unsigned status );
};

#endif // __Tableau_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
