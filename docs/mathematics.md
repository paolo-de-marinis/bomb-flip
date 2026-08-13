# Mathematics and design: Bomb Flip and Voltorb Flip

Bomb Flip is directly inspired by Voltorb Flip, the minigame in the non-Japanese versions of *Pokémon HeartGold* and *SoulSilver*. Both games use hidden values $0,1,2,3$, give a value sum and hazard count for every row and column, and complete a board after every ×2 and ×3 has been revealed.

This note separates that inherited model from the rules designed for Bomb Flip. Bomb Flip behavior is taken from the C source in this repository. Details of the original game are checked against the [pret/pokeheartgold community decompilation](https://github.com/pret/pokeheartgold/tree/master/src/voltorb_flip) and the sources listed at the end.

## 1. Shared model and Bomb Flip changes

| Inherited from Voltorb Flip | Designed for Bomb Flip |
| --- | --- |
| hidden 0/1/2/3 grid | additive card score |
| row and column value sums | countdown and time rewards |
| row and column hazard counts | scanner previews |
| reveal all ×2 and ×3 cards | Fold for half of the current level |
| zero-value hazard ends the round/run | twelve sequential levels |
| one level-1 composition | 6 × 6 boards on levels 9–12 |

Bomb Flip does not reproduce Voltorb Flip's multiplicative payout, memo pad, eight-level progression, mixture of board types or rejection of boards judged too easy.

## 2. Board and clues

For a square board of side $n$, let

~~~math
A=(a_{ij})\in\{0,1,2,3\}^{n\times n},
~~~

where 0 is a bomb and 1, 2 and 3 are coin cards. Voltorb Flip always has $n=5$. Bomb Flip uses $n=5$ on levels 1–8 and $n=6$ on levels 9–12.

Let $B,X_1,X_2,X_3$ be the numbers of the four card types and $N=n^2$. Then

~~~math
B+X_1+X_2+X_3=N,
\qquad
X_1=N-B-X_2-X_3.
~~~

For row $i$, the displayed clues are

~~~math
s_i=\sum_{j=1}^{n}a_{ij},
\qquad
b_i=\sum_{j=1}^{n}[a_{ij}=0].
~~~

For column $j$,

~~~math
c_j=\sum_{i=1}^{n}a_{ij},
\qquad
d_j=\sum_{i=1}^{n}[a_{ij}=0].
~~~

The bracket is 1 when the condition is true and 0 otherwise. `board_calculate_clues()` computes these four arrays with nested loops.

![Worked 6 × 6 Bomb Flip clue model](media/bomb-flip-clue-model.svg)

The diagram exposes one level-9-compatible board so the clues can be checked. It is a mathematical example, not a gameplay screenshot.

### Whole-board checks

Rows and columns count the same values:

~~~math
\sum_i s_i=\sum_j c_j
=X_1+2X_2+3X_3
=N-B+X_2+2X_3.
~~~

The bomb totals satisfy

~~~math
\sum_i b_i=\sum_j d_j=B.
~~~

For level 1, $N=25$, $B=6$, $X_2=3$ and $X_3=1$. Hence $X_1=15$, the sum of all values is 24 and the total bomb count is 6. The host tests verify these identities for all twelve configurations.

## 3. What a line clue determines

Consider one row or column of length $n$, with bomb count $b$ and value sum $s$. Let $n_1,n_2,n_3$ count its safe values. There are

~~~math
r=n-b=n_1+n_2+n_3
~~~

safe cards. If they were all ×1, their sum would be $r$. The excess over that baseline is

~~~math
e=s-r=s+b-n.
~~~

Since a ×2 contributes one unit above the baseline and a ×3 contributes two,

~~~math
e=n_2+2n_3.
~~~

Therefore

~~~math
\max(0,e-r)\leq n_3\leq\left\lfloor\frac e2\right\rfloor,
~~~

with

~~~math
n_2=e-2n_3,\qquad n_1=r-n_2-n_3.
~~~

This describes every line composition compatible with the two displayed clues.

### Example on a 6 × 6 board

Suppose $n=6$, $b=2$ and $s=7$. Then

~~~math
r=4,\qquad e=3.
~~~

Two safe multisets are possible:

| $n_1$ | $n_2$ | $n_3$ | Complete line |
| ---: | ---: | ---: | --- |
| 1 | 3 | 0 | $\{0,0,1,2,2,2\}$ |
| 2 | 1 | 1 | $\{0,0,1,1,2,3\}$ |

The line clue alone cannot choose between them. Intersecting clues and revealed cells provide the missing information.

## 4. Dead lines and the 6 × 6 extension

A line is dead when none of its unrevealed cells can contain a required ×2 or ×3. If $q_2$ twos and $q_3$ threes are already revealed, the remaining excess is

~~~math
e_{\mathrm{rem}}=s+b-n-q_2-2q_3.
~~~

The line is dead exactly when $e_{\mathrm{rem}}=0$.

For a 5 × 5 board:

~~~math
s+b-5=q_2+2q_3.
~~~

For Bomb Flip's 6 × 6 levels:

~~~math
s+b-6=q_2+2q_3.
~~~

Thus the familiar shortcut $s+b=5$ becomes $s+b=6$ on the larger board when no high card has yet been revealed. The clue system is unchanged; only the line length changes.

## 5. Completion

Let $R$ be the set of revealed cells. The board is complete when

~~~math
\{(i,j):a_{ij}\in\{2,3\}\}\subseteq R.
~~~

Revealing every ×1 is optional. In the C code, `board_all_high_cards_flipped()` implements this condition directly.

## 6. Bomb Flip generator

Each level has one fixed tuple $(B,X_2,X_3)$. `board_clear()` sets every active cell to 1. The placement routine then chooses distinct random cells for the bombs, ×2 cards and ×3 cards.

If `riv_rand_uint()` is uniform on the requested range, the routine samples the categorical arrangements uniformly. The number of layouts is

~~~math
M=\frac{N!}{B!\,X_1!\,X_2!\,X_3!}.
~~~

Before any clue is observed, a fixed position has marginals

~~~math
P(a_{ij}=0)=\frac BN,
\qquad
P(a_{ij}=k)=\frac{X_k}{N},\quad k=1,2,3.
~~~

Scanner positions are selected after the board. They must be safe and distinct, so they do not alter the board distribution. After one hundred failed random attempts, a row-major fallback guarantees that assignment finishes.

Voltorb Flip has a more involved generator. The reconstructed source uses several board configurations per level and rejects arrangements with too many risk-free multipliers. Exact probabilities for the original game therefore depend on the hidden board type and its acceptance rule; Bomb Flip has no corresponding filter.

## 7. Conditional probability

Let $H$ contain all visible clues and revealed cells. For Bomb Flip, let $N_H$ be the number of fixed-count layouts compatible with $H$, and $N_{ij,v}$ the number with $a_{ij}=v$. Under the generator above,

~~~math
P(a_{ij}=v\mid H)=\frac{N_{ij,v}}{N_H}.
~~~

Rows and columns cannot be treated as independent because every cell belongs to both. A solver can enumerate compatible assignments by backtracking over the joint constraint system. Bomb Flip does not implement such a solver; the equation describes the deduction problem faced by the player.

For comparison, an exact model of Voltorb Flip also needs a board-type variable because its hidden configurations and rejection rules have different weights.

## 8. Score, time and stopping

If $q_1,q_2,q_3$ safe cards have been revealed in the current Bomb Flip level, the card score is

~~~math
C_{\mathrm{cards}}=100(q_1+2q_2+3q_3).
~~~

Level $\ell$ starts with

~~~math
t_0=45+5(\ell-1)
~~~

seconds. A safe value $v$ adds $3v$ seconds, capped at 150. On completion, the game adds

~~~math
C_{\mathrm{time}}=\lfloor 10t\rfloor,
~~~

where $t$ is the remaining time.

Fold terminates the run and banks

~~~math
C_{\mathrm{fold}}=\left\lfloor\frac{C_{\mathrm{cards}}}{2}\right\rfloor
~~~

from the current level, in addition to earlier completed levels.

A bomb ends the run without banking the current level, but preserves earlier completed levels. Timeout is more severe and resets the entire run score to zero.

Scanner previews and Fold confirmation pause the countdown. This was a deliberate rule: using an information reward or considering the stopping decision does not itself consume time.

Voltorb Flip uses a multiplicative payout, $2^{q_2}3^{q_3}$, and has no countdown. Bomb Flip therefore changes the risk model from multiplication alone to an additive score coupled to time and an explicit stopping choice.

### Bellman interpretation

The choice between revealing a card, using the scanner and folding can be written as a small Bellman equation. Let

~~~math
z=(\ell,H,t,B,S,u)
~~~

describe the current level, the visible information, the remaining time, the score already banked, the score of the current level and the remaining scanner uses. Folding has the certain value

~~~math
F(z)=B+\left\lfloor\frac{S}{2}\right\rfloor.
~~~

For a hidden cell $x$, the expected value of revealing it is

~~~math
Q_{\mathrm{reveal}}(z,x)
=P(a_x=0\mid H)B
+\sum_{v=1}^{3}P(a_x=v\mid H)V(T_v(z,x)),
~~~

where $T_v$ is the state reached after revealing a safe value $v$. If $Q_{\mathrm{scan}}(z,x)$ also includes the value of the information shown by the scanner, the decision has the form

~~~math
V(z)=\max\left\{
F(z),
\max_x Q_{\mathrm{reveal}}(z,x),
\max_x Q_{\mathrm{scan}}(z,x)
\right\}.
~~~

The cartridge does not calculate this equation. It is only a compact description of the decision created by clues, time, scanner uses and Fold.

## 9. Level table

![Bomb Flip level composition and bomb density](media/bomb-flip-level-progression.svg)

| Level | Grid | ×1 | ×2 | ×3 | Bombs | Bomb density |
| ---: | :---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 5 × 5 | 15 | 3 | 1 | 6 | 24.0% |
| 2 | 5 × 5 | 12 | 4 | 2 | 7 | 28.0% |
| 3 | 5 × 5 | 9 | 5 | 3 | 8 | 32.0% |
| 4 | 5 × 5 | 8 | 6 | 3 | 8 | 32.0% |
| 5 | 5 × 5 | 4 | 7 | 4 | 10 | 40.0% |
| 6 | 5 × 5 | 3 | 8 | 4 | 10 | 40.0% |
| 7 | 5 × 5 | 2 | 8 | 5 | 10 | 40.0% |
| 8 | 5 × 5 | 0 | 10 | 5 | 10 | 40.0% |
| 9 | 6 × 6 | 13 | 7 | 3 | 13 | 36.1% |
| 10 | 6 × 6 | 11 | 8 | 3 | 14 | 38.9% |
| 11 | 6 × 6 | 9 | 9 | 3 | 15 | 41.7% |
| 12 | 6 × 6 | 7 | 10 | 3 | 16 | 44.4% |

Level 1 exactly matches one original Voltorb Flip level-1 composition. Levels 2–8 use different tuples, and the four 6 × 6 configurations have no original analogue.

The change from level 8 to 9 is a structural reset rather than a monotone increase in every statistic: the board grows, ×1 cards return and bomb density temporarily falls before rising again.

## 10. Constraint formulation

For a solver-oriented view, introduce binary variables

~~~math
x_{ij}^{(v)}\in\{0,1\},
\qquad v\in\{0,1,2,3\},
~~~

where $x_{ij}^{(v)}=1$ exactly when cell $(i,j)$ has value $v$. Each cell satisfies

~~~math
\sum_{v=0}^{3}x_{ij}^{(v)}=1.
~~~

The row clues become

~~~math
\sum_j\sum_{v=0}^{3}v\,x_{ij}^{(v)}=s_i,
\qquad
\sum_jx_{ij}^{(0)}=b_i.
~~~

Columns have the analogous equations, and the level tuple supplies global card-count constraints. This formulation explains why deductions must combine row, column and whole-board information.

The shipped cartridge does not solve this system. It stores the integer matrix and computes its clues directly.

## 11. Design and authorship boundary

The clue equations, card alphabet, completion condition and one level-1 composition come from Voltorb Flip and are credited as such.

Paolo De Marinis designed Bomb Flip's timer, additive score, scanner rewards, Fold rule, twelve-level progression, later level tuples and 6 × 6 extension. He wrote most of the original gameplay code and integrated, tested and refined the complete cartridge; Cursor was used mainly for animation work and as implementation support.

## 12. Sources

- [Original Bomb Flip cartridge on RIVES](https://app.rives.io/cartridges/5932d82f5827) — publication and provenance.
- [Bomb Flip `board.c`](../src/board.c) and [`game.c`](../src/game.c) — implemented Bomb Flip rules.
- [pret/pokeheartgold: game-state reconstruction](https://github.com/pret/pokeheartgold/blob/90e85d4e027f5e04800e7e015b3207094061402c/src/voltorb_flip/voltorb_flip_game.c) — original board configurations, generator, clues and progression data.
- [pret/pokeheartgold: application reconstruction](https://github.com/pret/pokeheartgold/blob/90e85d4e027f5e04800e7e015b3207094061402c/src/voltorb_flip/voltorb_flip.c) — Quit, memo and round flow.
- [Bulbapedia: Voltorb Flip](https://bulbapedia.bulbagarden.net/wiki/Voltorb_Flip) — rules, compositions and dead-line formula.
- [Marcus Pasell: algorithmic solution](https://understandable.dev/deep-dives/voltorb-flip/) — compatible-board enumeration and row/column dependence.
- [The Cave of Dragonflies: Voltorb Flip guide](https://www.dragonflycave.com/johto/voltorb-flip/) — worked clue deductions.
- [GameFAQs: Voltorb Flip guide](https://gamefaqs.gamespot.com/ds/960100-pokemon-soulsilver-version/faqs/59308) — classic five-cell dead-line rule.
- [Gimmy Tomas: solver explanation](https://gimmytomas.github.io/voltorb-flip/algorithm.html) — Bayesian treatment of original board types; cited as solver methodology, not as Bomb Flip implementation evidence.
