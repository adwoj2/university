#!/bin/bash

#zmienne dotyczące kolorów
red=`tput setaf 1`
green=`tput setaf 2`
white=`tput setaf 7`

#sprawdzenie poprawnosci danych wejsciowych

if [ $# != 2 ]
then
    echo "Niewlasciwa ilosc argumentow $#. Prawidłowy format to $0 <path> <dir>."
    exit 1 
fi

for f in $2/*.in
do
    #sprawdzenie poprawności wyników
    $1 < $f >  wynik.out 2> wynik.err
    EXITCODE=$?
    if diff wynik.out ${f%in}out >/dev/null 2>&1  
    then
        if diff wynik.err ${f%in}err >/dev/null 2>&1
        then
	        echo "${white}Test $f zakończył się ${green}powodzeniem."
        else
            echo "${white}Test $f zakończył się  ${red}niepowodzeniem."
        fi
    else
	echo "${white}Test $f zakończył się ${red}niepowodzeniem."
    fi
    echo -e "${white}Program zakończył się kodem $EXITCODE."
    
    #sprawdzenie obsługi pamięci pod valgrindem
    valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all $1 < $f > wynik.out 2> wynik.err
    EXITCODE=$?
    if ((EXITCODE == 123))
    then
        echo "${white}Test pamięci $f pod valgrindem zakończył się ${red}niepowodzeniem."
    else
        echo "${white}Test pamięci $f pod valgrindem zakończył się  ${green}powodzeniem."
    fi
    echo -e "${white}Program zakończył się kodem $EXITCODE.\n"
    rm wynik.out
    rm wynik.err
done