Morple

Morple is a rock-paper-scissors AI. Morple has six predictors:

1. A unigram predictor, which counts occurrences of plays in the opponent
2. A Markov modeler, which constructs a Markov model of the opponents' plays
3. A predictor which send _our_ previous move to the opponent's next move (called "Self Markov modeler" in the source)
4. A predictor which sends the _pair of moves_ last turn to the opponent's next move (called "Dual Markov modeler" in the source)
5. A long pattern matcher, which looks for the previous _n_ moves with n from 1 to 25 in the opponent's last 900 moves, occurrences weighted by the square of their length
6. A similar long pattern matcher, which send _our_ patterns to the opponent's next move (this is for playing against strong players, who will of course be looking at _our_ history, not their own).

Morple leads with random moves until her estimated expectation (win probability - lose probability) exceeds a magic number (current 0.15). Morple then weights these predictors according to their individual estimated expectations. If we would be better off betting _against_ any predictor (e.g. if the opponent has discovered our strategy and is beating it consistently), we do so (by adding to its results modulo three). Then, if her estimated expectation drops back below the magic number, she falls back on random again.
