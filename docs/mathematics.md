# Mathematics of the Bomb Flip state machine

Bomb Flip is easiest to describe by separating three objects that the code keeps distinct:

1. the hidden board and the clues derived from it;
2. the mutable run state carried from frame to frame;
3. the transition selected by a reveal, scanner use, Fold, completion or timeout.

The mathematical path is therefore

~~~math
\boxed{
\text{board generation}
\longrightarrow
\text{clue map}
\longrightarrow
\text{game state}
\longrightarrow
\text{state transition}.
}
~~~

This document derives those objects from [`board.c`](../src/board.c),
[`game.c`](../src/game.c) and [`state.h`](../src/state.h). Probability conditioned on player
knowledge and a Bellman interpretation are kept in separate appendices because the cartridge
does not implement either a solver or an optimal-policy calculation.

Bomb Flip is inspired by Voltorb Flip from *Pokémon HeartGold* and *SoulSilver*. The historical
relationship is summarized near the end; the main derivation below concerns Bomb Flip's current
code.

## 1. Hidden board

At level $\ell$, the active board has side length

~~~math
n(\ell)=
\begin{cases}
5, & 1\leq\ell\leq8,\\
6, & 9\leq\ell\leq12.
\end{cases}
~~~

Write

~~~math
A=(a_{ij})\in\{0,1,2,3\}^{n\times n},
~~~

where

- $0$ is a bomb;
- $1$ is a ×1 card;
- $2$ is a ×2 card;
- $3$ is a ×3 card.

The fixed-capacity C array is always $6\times6$, but cells outside the active $n\times n$ region
are assigned value $-1$ and do not belong to the mathematical board.

For one level let

~~~math
X_0,\quad X_1,\quad X_2,\quad X_3
~~~

be the counts of values $0,1,2,3$, respectively, and let $N=n^2$. Then

~~~math
X_0+X_1+X_2+X_3=N,
\qquad
X_1=N-X_0-X_2-X_3.
~~~

`LEVEL_CONFIGS` stores $(X_2,X_3,X_0)$; $X_1$ is therefore determined rather than stored.

## 2. Board generation as a fixed-count random arrangement

`board_clear()` first sets every active cell to $1$. `place_level_cards()` then places, in order,

~~~math
X_0\text{ bombs},\qquad X_2\text{ twos},\qquad X_3\text{ threes}
~~~

at random currently unused cells. The remaining cells stay equal to $1$.

If each `riv_rand_uint()` call is uniform over the requested coordinate range, rejection of
already occupied cells is equivalent to choosing each next free position uniformly from the
remaining cells. Under that assumption the generated categorical layouts are uniform over

~~~math
\Omega_\ell
=
\left\{
A\in\{0,1,2,3\}^{n\times n}:
\#0=X_0,\ \#1=X_1,\ \#2=X_2,\ \#3=X_3
\right\}.
~~~

Their number is

~~~math
|\Omega_\ell|
=
\frac{N!}{X_0!\,X_1!\,X_2!\,X_3!}.
~~~

Before clues or reveals are considered, every fixed cell therefore has marginals

~~~math
\Pr[a_{ij}=k]=\frac{X_k}{N},
\qquad k=0,1,2,3,
~~~

under the same uniform-RNG assumption.

This is a property of the board generator. The cartridge does not enumerate $\Omega_\ell$ or
compute these probabilities at runtime.

## 3. Clues are a deterministic map of the board

After the values are placed, `board_calculate_clues()` computes four arrays. For row $i$,

~~~math
s_i=\sum_{j=1}^{n}a_{ij},
\qquad
b_i=\sum_{j=1}^{n}[a_{ij}=0],
~~~

and for column $j$,

~~~math
c_j=\sum_{i=1}^{n}a_{ij},
\qquad
d_j=\sum_{i=1}^{n}[a_{ij}=0].
~~~

Here $[P]$ is $1$ when proposition $P$ is true and $0$ otherwise.

Thus the displayed clue data is a deterministic map

~~~math
\Gamma:A\longmapsto(s,b,c,d).
~~~

The board remains hidden; the clue arrays expose only these row and column aggregates.

### 3.1 Whole-board identities

Rows and columns count the same values, so

~~~math
\sum_i s_i
=
\sum_j c_j
=
X_1+2X_2+3X_3
=
N-X_0+X_2+2X_3.
~~~

Likewise,

~~~math
\sum_i b_i
=
\sum_j d_j
=X_0.
~~~

These identities are checked by the host tests for all twelve level configurations.

![Worked 6 x 6 Bomb Flip clue model](media/bomb-flip-clue-model.svg)

The diagram is a mathematical example compatible with level 9, not a screenshot of a fully
revealed run.

## 4. What one line clue determines

Consider one row or column of length $n$ with value sum $s$ and bomb count $b$. Let
$n_1,n_2,n_3$ be its counts of safe values. The number of safe cells is

~~~math
r=n-b=n_1+n_2+n_3.
~~~

If all safe cells were ×1, their sum would be $r$. Define the excess over that baseline by

~~~math
e=s-r=s+b-n.
~~~

A ×2 contributes one excess unit and a ×3 contributes two, hence

~~~math
e=n_2+2n_3.
~~~

Every line composition compatible with the two clues therefore satisfies

~~~math
\max(0,e-r)
\leq n_3\leq
\left\lfloor\frac e2\right\rfloor,
~~~

with

~~~math
n_2=e-2n_3,
\qquad
n_1=r-n_2-n_3.
~~~

The clue pair generally determines a set of possible compositions rather than a unique one.

### Example

For $n=6$, $b=2$ and $s=7$,

~~~math
r=4,
\qquad
e=3.
~~~

Two compatible safe compositions are

| $n_1$ | $n_2$ | $n_3$ | Complete line |
| ---: | ---: | ---: | --- |
| 1 | 3 | 0 | $\{0,0,1,2,2,2\}$ |
| 2 | 1 | 1 | $\{0,0,1,1,2,3\}$ |

The code displays the clues; it does not run this deduction for the player.

## 5. Revealed set and completion predicate

Let

~~~math
R\subseteq\{1,\ldots,n\}^2
~~~

be the set of cells whose `revealed` flag is true. The set of high-value cells is

~~~math
M(A)=\{(i,j):a_{ij}\in\{2,3\}\}.
~~~

`board_all_high_cards_flipped()` implements exactly

~~~math
\boxed{M(A)\subseteq R.}
~~~

A board can therefore complete while some ×1 cards and bombs remain unrevealed.

For a single line, if $q_2$ twos and $q_3$ threes on that line are already revealed, the
remaining excess is

~~~math
e_{\mathrm{rem}}
=
s+b-n-q_2-2q_3.
~~~

When the current state is consistent, the unrevealed part of the line contains no remaining ×2
or ×3 exactly when

~~~math
e_{\mathrm{rem}}=0.
~~~

The familiar initial shortcut $s+b=n$ is the special case $q_2=q_3=0$; it is $s+b=5$ on the
first eight levels and $s+b=6$ on the last four.

## 6. Scanner rewards are hidden metadata on safe cells

Scanner rewards are assigned only after $A$ and its clues have been generated. Let

~~~math
G\subseteq\{(i,j):a_{ij}>0\}
~~~

be the set of designated reward cells. Its size is

~~~math
|G|=
\begin{cases}
0, & 1\leq\ell\leq3,\\
1, & 4\leq\ell\leq8,\\
2, & 9\leq\ell\leq12.
\end{cases}
~~~

The two reward cells, when present, are distinct. Their designation does not change $A$ and is
not included in the row or column clues.

`board_assign_scanner_tiles()` first tries random safe cells. After one hundred unsuccessful
attempts for one reward, it chooses the first available safe cell in row-major order. Therefore
$G$ is hidden metadata attached to safe cells, but its implementation should not be described as
an independent uniformly sampled subset without accounting for that fallback.

## 7. A mathematical projection of `GameState`

`GameState` contains animation and presentation fields in addition to gameplay state. For the
rules below, the relevant projection is

~~~math
\sigma
=
(\ell,A,R,G,B,S,t,u,\varphi),
~~~

where

- $\ell\in\{1,\ldots,12\}$ is the current level;
- $A$ is the current hidden board;
- $R$ is the revealed set;
- $G$ is the scanner-reward set;
- $B$ is `totalCoins`, score already banked from earlier completed levels;
- $S$ is `levelCoins`, score accumulated on the current level;
- $t$ is `timeRemaining`;
- $u$ is `scannerUses`;
- $\varphi$ is the current `GamePhase`.

The change of notation is deliberate: $X_0$ denotes the number of bombs on a board, while $B$
from this point onward denotes banked score.

This tuple is not a replacement C type. It is the smallest state needed to write the scoring,
timing and terminal transitions without carrying rendering counters.

A new level resets

~~~math
S=0,
\qquad
u=0,
\qquad
R=\varnothing,
\qquad
\varphi=\mathrm{ACTIVE},
~~~

and initializes

~~~math
t_0(\ell)=45+5(\ell-1).
~~~

The banked score $B$ survives ordinary progression to the next level.

## 8. Active time evolves before ordinary input

On an ordinary active frame, `update_timer()` first applies

~~~math
t\leftarrow t-\frac1{60}.
~~~

If the result is non-positive, the game enters `GAME_PHASE_TIMEOUT_CHAIN` and ordinary board
input for that frame does not run.

This order matters: the timer transition precedes reveal, Fold and scanner handling in the
active update.

Two modal interfaces are exceptions to the ordinary outer-frame evolution. Fold confirmation
and scanner preview execute their own `riv_present()` loops. While those loops are active,
`game_update()` is not called, so the mathematical game timer $t$ remains unchanged during the
nested modal frames. The ordinary $1/60$-second decrement for the frame that opened the modal
has already happened before input handling.

## 9. Reveal is a state transition, not just a visual flip

Suppose the game is active and the selected unrevealed cell is $x$. Let

~~~math
v=a_x\in\{0,1,2,3\}.
~~~

The reveal first updates

~~~math
R' = R\cup\{x\}.
~~~

If $x\in G$, then $x$ is safe by construction and the scanner-use count becomes

~~~math
u'=u+v.
~~~

Otherwise $u'=u$.

The remaining transition depends on $v$.

### 9.1 Bomb reveal

For

~~~math
v=0,
~~~

no score or time reward is added. The phase becomes `GAME_PHASE_BOMB_REVEAL`; after the reveal
and explosion presentation finishes, `game_finish(..., GAME_END_BOMB, ...)` terminates the run.
The final banked score is

~~~math
\boxed{B.}
~~~

The current $S$ is never transferred into `totalCoins`.

### 9.2 Safe reveal

For

~~~math
v\in\{1,2,3\},
~~~

the code applies

~~~math
S' = S+100v
~~~

and

~~~math
t'
=
\min(150,t+3v).
~~~

If the new revealed set does not yet contain every ×2 and ×3 card, the game remains active with
these updated values.

### 9.3 Revealing the last required high card

If

~~~math
M(A)\subseteq R',
~~~

then the current level score is banked immediately:

~~~math
B_1=B+S'.
~~~

The phase changes to `GAME_PHASE_LEVEL_CLEARING`. After the remaining cards have been revealed
for presentation and the clear delay expires, the time bonus is computed as

~~~math
C_{\mathrm{time}}
=
\lfloor10t'\rfloor
~~~

and the banked score becomes

~~~math
\boxed{B'=B+S'+\lfloor10t'\rfloor.}
~~~

For $\ell<12$, this value is carried into the next level. At level 12 it becomes the completed
run score.

The presentation phase therefore delays the time bonus, but it does not return the game to an
ordinary decision state between completion detection and bonus application.

## 10. Fold is a deterministic terminal transition

While the game is active, opening the Fold dialog does not itself alter $B$ or $S$ and adds no
extra timer decrement during its nested modal frames. Confirming Fold terminates the run and
computes

~~~math
\boxed{
B_{\mathrm{fold}}
=
B+\left\lfloor\frac S2\right\rfloor.
}
~~~

The floor is ordinary integer division on the non-negative current-level score.

Fold therefore differs from a bomb in one precise way: both preserve previously banked score,
but Fold transfers half of the exposed current-level score before termination.

## 11. Timeout is a delayed presentation of a zero-score terminal state

When the active timer reaches zero, the game enters a timeout chain that reveals bombs one by
one. This chain is a presentation phase; it does not restore ordinary input.

After the chain finishes, `GAME_END_TIMEOUT` sets

~~~math
t=0,
\qquad
B=0,
\qquad
S=0.
~~~

Thus the final timeout score is

~~~math
\boxed{0,}
~~~

including the score banked on earlier levels.

This differs from bomb termination, which preserves $B$.

## 12. Scanner preview changes neither the board value nor the revealed set

A scanner use is available only while the phase is active, at least one use remains and the
board is not already complete. If the selected cell is unrevealed, `use_scanner()` temporarily
animates its face, then restores the face-down state and applies

~~~math
u\leftarrow u-1.
~~~

Crucially,

~~~math
R' = R.
~~~

The `revealed` flag is never set by preview. A ×2 or ×3 seen through the scanner must therefore
still be revealed normally before the completion predicate can become true.

The code also does not store a permanent memo of the previewed value. Any information retained
after the card turns face down belongs to the human player's memory, not to `GameState`.

## 13. `GamePhase` makes animation part of the control state

The gameplay phases are

~~~math
\begin{aligned}
&\mathrm{ACTIVE},\ \mathrm{BOMB\_REVEAL},\ \mathrm{TIMEOUT\_CHAIN},\\
&\mathrm{LEVEL\_CLEARING},\ \mathrm{LEVEL\_CLEARED},
\ \mathrm{NEXT\_LEVEL},\ \mathrm{FINISHED}.
\end{aligned}
~~~

`game_update()` dispatches on this phase before ordinary active input is processed. Therefore
bomb animation, timeout explosions and level-clear presentation are not cosmetic overlays on an
otherwise active board: they are separate control states that suppress the ordinary transition
rules until they finish.

The run can be summarized as

~~~math
\boxed{
\sigma_n
\xrightarrow{\text{timer}}
\xrightarrow{\text{active action or phase update}}
\sigma_{n+1},
}
~~~

with terminal branches for Bomb, Fold, Timeout and final completion.

## 14. Level configurations

![Bomb Flip level composition and bomb density](media/bomb-flip-level-progression.svg)

| Level | Grid | ×1 | ×2 | ×3 | Bombs | Scanner rewards | Bomb density |
| ---: | :---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 5 x 5 | 15 | 3 | 1 | 6 | 0 | 24.0% |
| 2 | 5 x 5 | 12 | 4 | 2 | 7 | 0 | 28.0% |
| 3 | 5 x 5 | 9 | 5 | 3 | 8 | 0 | 32.0% |
| 4 | 5 x 5 | 8 | 6 | 3 | 8 | 1 | 32.0% |
| 5 | 5 x 5 | 4 | 7 | 4 | 10 | 1 | 40.0% |
| 6 | 5 x 5 | 3 | 8 | 4 | 10 | 1 | 40.0% |
| 7 | 5 x 5 | 2 | 8 | 5 | 10 | 1 | 40.0% |
| 8 | 5 x 5 | 0 | 10 | 5 | 10 | 1 | 40.0% |
| 9 | 6 x 6 | 13 | 7 | 3 | 13 | 2 | 36.1% |
| 10 | 6 x 6 | 11 | 8 | 3 | 14 | 2 | 38.9% |
| 11 | 6 x 6 | 9 | 9 | 3 | 15 | 2 | 41.7% |
| 12 | 6 x 6 | 7 | 10 | 3 | 16 | 2 | 44.4% |

These values are the fixed `LEVEL_CONFIGS` used by the current maintained source.

## 15. Relation to Voltorb Flip

Bomb Flip keeps several structural ideas from Voltorb Flip:

- hidden values $0,1,2,3$;
- row and column value sums;
- row and column bomb counts;
- completion after every ×2 and ×3 is revealed;
- one level-1 composition.

The current Bomb Flip code separately defines its additive score, countdown, time rewards,
scanner metadata, Fold transition, twelve-level progression and 6 x 6 extension.

Bomb Flip does not implement Voltorb Flip's multiplicative payout, memo pad, eight-level
progression, family of board types or board-acceptance filter. Historical and external references
are collected separately in [Sources and provenance](references.md).

Bomb Flip is an independent, unaffiliated cartridge. Pokémon and related names belong to their
respective rights holders.

## 16. Limits of the mathematical description

The formulas above are a derivation of the current implementation. They do not claim that the
cartridge solves the inference problem implied by its clues, computes optimal actions or proves
a difficulty property for the fixed level table.

The code-level distinctions that should remain explicit are:

- board values $A$ and scanner metadata $G$ are separate;
- scanner preview does not modify $R$;
- $B$ and $S$ have different exposure to terminal outcomes;
- `GamePhase` is part of the transition system, not merely a rendering label;
- modal Fold and scanner loops consume presentation frames without additional timer decrements;
- the stochastic statements about board layouts assume uniform `riv_rand_uint()` output.

Automated evidence for these rules is summarized in [Validation](validation.md).

---

## Appendix A. Player inference as a mathematical curiosity

The clues and revealed cards naturally define an inference problem, but Bomb Flip does not solve
it internally.

Let $H$ denote information available to an external observer: displayed clues together with
revealed values and any previewed value the player remembers. Let $\Omega_{\ell,H}\subseteq
\Omega_\ell$ be the fixed-count boards compatible with that information. Under the uniform-board
assumption from Section 2,

~~~math
\Pr(a_{ij}=v\mid H)
=
\frac{
|\{A\in\Omega_{\ell,H}:a_{ij}=v\}|
}{
|\Omega_{\ell,H}|
}.
~~~

Rows and columns cannot generally be treated as independent because every cell participates in
one constraint of each type. An external solver could enumerate compatible boards by
backtracking, but no such enumeration appears in the cartridge.

The distinction from Section 12 matters: a scanner preview may enlarge the human information
state $H$ while leaving the code's revealed set $R$ unchanged.

## Appendix B. Bellman interpretation as a further curiosity

If one augments the implemented state with an external information state $H$, the choice between
revealing, scanning and folding can be interpreted as a finite-horizon decision problem.

For example, Fold has deterministic terminal value

~~~math
F(\sigma)
=
B+\left\lfloor\frac S2\right\rfloor.
~~~

One could then define expected values for reveal and scanner actions from conditional
probabilities such as those in Appendix A and write schematically

~~~math
V(\sigma,H)
=
\max
\left\{
F(\sigma),
\max_x Q_{\mathrm{reveal}}(\sigma,H,x),
\max_x Q_{\mathrm{scan}}(\sigma,H,x)
\right\}.
~~~

This is **not** an algorithm implemented by Bomb Flip, and the cartridge neither computes
$V$ nor stores the full $H$ needed by such a model. It is included only to show how the code's
actual transition rules can be embedded in a larger decision-theoretic interpretation.