Project: Multi-user On-line Game
Author:  通訊所一 610430008 楊博禕

#How to start?
    type in commands:
    1 ) make
    2 ) server side: ./server
        client side: ./client server_ip server_port

#How to play?
    Before Login/Register:
        type in 'l'/'L' to login or 'r'/'R' to register a new account
        follow the format below:
            username  passwd

    After Login/Register:
    There are several general commands for client(remember to add ']' before commands!!)

        ========== USAGE ========== 
        ]setName newname
            -- set a new name
        ]look x y
            -- check players and items at the point you've specified
        ]inventory
            -- check what's in your backpack
        ]take something
            -- take a item at the point you're now standing at
        ]deposit something
            -- deposit what's in your backpack
        ]move dir
            -- move to another grid
        ]give someone something
            -- give someone something only if there's a something in your backpack and someone is beside you
        ]tell someone msg
            -- tell someone a secret
        ]who
            -- check who is in the game
        ]save
            -- save current game state
        ]exit
            -- exit game
        ]help
            -- get some hints

ENJOY IT :P