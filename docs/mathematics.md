# Mathematics of the board and clues

## Scope

Bomb Flip stores each level as a small matrix and derives all row and column clues from that matrix. The game does not contain an automatic solver: it exposes enough aggregate information for the player to reason about hidden cells while deciding whether to continue or fold.

This document describes the mathematical structure implemented in `src/bombflip.c` and distinguishes that structure from strategies a player might apply.

## 1. Board as a matrix

For a board of side (n), let

$$
A=(a_{ij})in{0,1,2,3}^{n	imes n},
$$

where

- (a_{ij}=0) represents a bomb;
- (a_{ij}=1) represents a x1 card;
- (a_{ij}=2) represents a x2 card;
- (a_{ij}=3) represents a x3 card.

Levels 1–8 use (n=5); levels 9–12 use (n=6).

Let (B,X_1,X_2,X_3) be the numbers of the four card types. With (N=n^2),

$$
B+X_1+X_2+X_3=N.
$$

The implementation initializes every active cell to 1, then replaces distinct cells with all bombs, x2 cards and x3 cards required by the level configuration. Therefore

$$
X_1=N-B-X_2-X_3.
$$

## 2. Row and column clues

For row (i), the game displays the value sum

$$
s_i=sum_{j=1}^{n}a_{ij}
$$

and the bomb count

$$
b_i=#{j:a_{ij}=0}.
$$

For column (j), it analogously displays

$$
c_j=sum_{i=1}^{n}a_{ij},qquad
d_j=#{i:a_{ij}=0}.
$$

These values are calculated directly by `calculateClues()`; they are not stored as independent puzzle data. This guarantees that every displayed clue is consistent with the generated board.

## 3. Global consistency invariants

The row sums and column sums count the same cells, so every generated board satisfies

$$
sum_{i=1}^{n}s_i
=
sum_{j=1}^{n}c_j
=
X_1+2X_2+3X_3.
$$

Substituting (X_1=N-B-X_2-X_3) gives the useful form

$$
sum_i s_i=sum_j c_j=N-B+X_2+2X_3.
$$

Bomb counts satisfy a second pair of invariants:

$$
sum_i b_i=sum_j d_j=B.
$$

These identities are useful both for understanding the puzzle and for validating a level generator: row and column totals must agree with each other and with the configured card counts.

## 4. What a single clue implies

Consider one row with (b) bombs and value sum (s). Let (n_1,n_2,n_3) be the numbers of x1, x2 and x3 cards in that row. The number of non-bombs is

$$
r=n-b=n_1+n_2+n_3.
$$

If every safe card were x1, the row sum would be (r). Define the excess over this baseline:

$$
e=s-r.
$$

Because x2 contributes one extra unit and x3 contributes two,

$$
e=n_2+2n_3.
$$

Consequently the possible compositions of the line are given by

$$
max(0,e-r)leq n_3leqleftlfloorrac{e}{2}ightfloor,
$$

with

$$
n_2=e-2n_3,qquad
n_1=r-n_2-n_3.
$$

The same reasoning applies to every column.

Example: on a 5-cell line with one bomb and sum 7, (r=4) and (e=3). The safe cells can be either three x2 cards plus one x1, or one x3 plus one x2 plus two x1 cards. The clue constrains the line but does not necessarily determine every card; intersections with column clues provide the additional information.

## 5. Level composition

The implemented level table changes board size and card composition as follows:

| Level | Grid | Cells (N) | x2 | x3 | Bombs (B) | x1 | Bomb density |
|---:|:---:|---:|---:|---:|---:|---:|---:|
| 1 | 5×5 | 25 | 3 | 1 | 6 | 15 | 24.0% |
| 2 | 5×5 | 25 | 4 | 2 | 7 | 12 | 28.0% |
| 3 | 5×5 | 25 | 5 | 3 | 8 | 9 | 32.0% |
| 4 | 5×5 | 25 | 6 | 3 | 8 | 8 | 32.0% |
| 5 | 5×5 | 25 | 7 | 4 | 10 | 4 | 40.0% |
| 6 | 5×5 | 25 | 8 | 4 | 10 | 3 | 40.0% |
| 7 | 5×5 | 25 | 8 | 5 | 10 | 2 | 40.0% |
| 8 | 5×5 | 25 | 10 | 5 | 10 | 0 | 40.0% |
| 9 | 6×6 | 36 | 7 | 3 | 13 | 13 | 36.1% |
| 10 | 6×6 | 36 | 8 | 3 | 14 | 11 | 38.9% |
| 11 | 6×6 | 36 | 9 | 3 | 15 | 9 | 41.7% |
| 12 | 6×6 | 36 | 10 | 3 | 16 | 7 | 44.4% |

Difficulty is not controlled by bomb density alone. The number of high cards affects the completion target, while the ratio of x1/x2/x3 cards changes how informative the sums are. Level 8 is an interesting boundary case: every non-bomb is a required x2 or x3 card.

## 6. Random placement model

`placeLevelCards()` repeatedly samples coordinates and accepts a cell only if it is still x1. Bombs are placed first, then x2 cards, then x3 cards. This is equivalent to drawing distinct positions without replacement, assuming `riv_rand_uint()` is uniform over the active coordinate range.

Under that assumption, every categorical board with the configured counts has the same probability, and the number of possible layouts is

$$
rac{N!}{B!,X_1!,X_2!,X_3!}.
$$

Before observing clues or revealed cells, the marginal probabilities for a uniformly selected position are

$$
P(	ext{bomb})=rac{B}{N},quad
P(	ext{x}k)=rac{X_k}{N}quad(k=1,2,3).
$$

Once row/column clues and reveals are known, cells are no longer independent. Correct risk estimates become conditional on all compatible board assignments. Bomb Flip deliberately leaves that inference to the player; there is no probability engine or constraint solver in the code.

## 7. Completion and the fold decision

A level is complete when every x2 and x3 card has been revealed. x1 cards are optional. In symbols, if (R) is the set of revealed cells, completion requires

$$
{(i,j):a_{ij}in{2,3}}subseteq R.
$$

This condition makes the puzzle asymmetric: the player is searching for all high-value cards, not clearing the whole matrix.

Folding introduces a stopping decision. Continuing may uncover required cards and increase the reward, but it also risks revealing a bomb; folding ends the run and retains half of the current level's coins. The code implements the rules and outcomes, not an optimal-stopping policy. Any expected-value strategy depends on the player's current information and risk preference.

## 8. Design choices and consequences

| Choice in Bomb Flip | Mathematical consequence | Gameplay effect |
|---|---|---|
| Values encoded as (0,1,2,3) | Sums and bomb counts are simple reductions of one matrix | Clues are compact and always checkable |
| Separate sum and bomb count per line | Each line supplies two different constraints | Players combine value information with hazard information |
| Fixed counts per level | Global invariants and a finite layout space | Difficulty can be tuned through composition |
| Random positions without overlap | Sampling without replacement | Every level changes while preserving its configured totals |
| Only x2/x3 required for completion | Completion is a subset condition | x1 cards may remain hidden |
| Manual reasoning, no solver | Code computes clues but not posterior probabilities | Deduction stays with the player |
| Fold for half the level coins | Creates a stopping/risk trade-off | A safe partial result competes with further progress |

## 9. Constraint-system view

The board can also be described as a finite constraint-satisfaction problem. Introduce binary variables

$$
x_{ij}^{(v)}in{0,1},qquad vin{0,1,2,3},
$$

where (x_{ij}^{(v)}=1) exactly when cell ((i,j)) contains value (v). Each cell obeys

$$
sum_{v=0}^{3}x_{ij}^{(v)}=1.
$$

For a row (i),

$$
sum_jsum_{v=0}^{3}v,x_{ij}^{(v)}=s_i,
qquad
sum_jx_{ij}^{(0)}=b_i,
$$

with analogous equations for columns and global equations for the configured card counts.

This formulation explains why row and column information combines naturally. It is an analysis model only: the shipped C implementation uses the direct integer matrix and simple loops, which are smaller and more appropriate for this game.

## 10. Development note

The matrix representation, clue system, card composition and stopping rules make the mathematical structure of the implemented game visible. Bomb Flip was developed in C with the RIVES library through a Cursor-assisted workflow. Paolo De Marinis conceived the mechanics and gameplay, directly wrote and modified parts of the code, and handled integration, testing, debugging and refinement.

This document describes the maintained implementation and does not use the later repository cleanup as evidence of historical authorship for individual source lines.

## 11. Relevant code

- `grid` in `GameState` — the active matrix.
- `LevelConfig` and `levelConfigs` — per-level x2, x3 and bomb counts.
- `clearGrid()` — initializes the active matrix to x1.
- `placeLevelCards()` — distinct random placement.
- `calculateClues()` — row/column sums and bomb counts.
- `allHighCardsFlipped()` — x2/x3 completion condition.
- fold handling — stopping outcome and retained level coins.
