#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define STRING_SOFT_VERSION "SDispenser v.1.2"
//Брой на бункерите със сътавки за дозиране. При промяна трябва да се променят и таблиците в БД
//Виж коментара в TODO.txt
#define TANKS_NUM  11

#define TICK_PERIOD 10 //Не бива да е повече от 1000мс

//Т.к. в рецептата има фиксиран брой съставки според броя на бункерите,
//то за да се правят рецепти с по-малък брой съставки в колоната medX_Id се
//записва това дефинирано число ако съставката не е включена
#define NONE_ID 1   //Id в таблицата tblMeds за записа за липсващо лекарство.
#define ADVANCE_DEFAULT_VALUE 1
#define COM_ERR_THRESHOLD     5

#define TEST_MODE

#endif // DEFINITIONS_H
