# Commandes via le PIPE

> Un argument vide doit être représenté par un espace

## ADD

`ADD|num?|dest|rebound|time|reset`

- num?: str
- dest: str
- rebound: bool
- time: int
- reset: bool

> Lorsque du texte peut être affiché à l'écran, il est possible d'utilier `\fn` pour changer la police, avec `n` correspondant à une police dans la commande `FNT`.

## SET

`SET|idx|num?|dest|rebound|time|reset`

- idx: int
- num?: str
- dest: str
- rebound: bool
- time: int
- reset: bool

## CLR

`CLR|`

## FNT

`FNT|id`

- id: int (0: lp_b (E5 ts), 1: lp_6 (petit), 2: lp_a (C20 petit))