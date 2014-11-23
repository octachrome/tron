My entry for the Tron competition on codingame.com. I peaked at 5h place during the last week, and ended in 13th place overall.

It started as a 4-way minimax algorithm, where each player tries to maximise their own minimum score while assuming that all the other players will do the same. I intended to add a kind of alpha-beta pruning, but it turned out to be quite difficult to determine when the tree can be safely pruned.

In the end I focused most of my time on improving the game evaluation function, which is based on dividing the board into regions controlled by each player and comparing their sizes. When analysing region size, it takes into account sub-regions which can be entered but not left (via narrow doorways). The scoring is based on which players will die in which order, and on who will probably die next based on the size of the regions that each live player controls.

The code is very poorly commented, due to time constraints, but there are a lot of unit tests covering the key scoring functions, etc., which were essential for proving that the code works correctly.
