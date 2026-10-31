#include "execute.h"

#include "instructions.h"
#include "exec_context.h"

// TODO: Reimplement
void debug_print(exec_context* c) {
    fprintf(stderr,"\nRound %i\n", _gs->round);

    switch (c->type) {
        case ENTITY_CONTEXT:
            switch (c->entity->type) {
                case ENTITY_PLAYER:
                    fprintf(stderr,"\nPlayer %s(%i)\n", c->entity->player->name, c->entity->id);
                    break;
            
                case ENTITY_VEHICLE:
                    break;
            }
            break;

        case EVENT_CONTEXT:
            fprintf(stderr,"\nEvent\n");
            break;
    }
    
    for(int i = 0; i < c->sp; i++) {
        fprintf(stderr,"%i, ", c->stack[i]); wait(0.1);
    }
    fprintf(stderr,"\ninstr:%i\ndp:%i\nbp:%i\nsp:%i\n\n", c->program[c->dp], c->dp, c->bp, c->sp); 
    wait(1);
}

void clean_up(exec_context* c) {
    switch (c->type) {
        case ENTITY_CONTEXT:
            switch (c->entity->type) {
                case ENTITY_PLAYER:
                    // TODO: Reset actions and steps here???
                    break;
            
                case ENTITY_VEHICLE:
                    break;
            }
            break;

        case EVENT_CONTEXT:
            switch (c->event->template->type) {
                case STATELESS_EVENT:
                    c->dp = 0;
                    c->bp = 0;
                    c->sp = 0;
                    memset(stateless_stack, 0, sizeof(int) * 100); // TODO: Get this from some shared place
                    break;

                case STATEFUL_EVENT:
                case SHARED_EVENT:
                    c->dp = 2; // Skip intial declarations
                    break;
            }
            break;
    }
}

void exec(exec_context* c) {
    while(1) {
        int change = 0;
        if (c->dp >= c->program_len) break;
        if (!use_resource(1, &c->steps)) break;
        //debug_print(c);
        // TODO: Re-add debug info, need more context?
        //_log(DEBUG, "%s executes %i", ps->name, ps->directive[ps->dp]);
        switch (c->program[c->dp++]) {
            case Instr_Wait: {
                if(!use_resource(1,&c->actions)) { c->dp--; goto end; }
                c->stack[c->sp++] = 1;
                break;
            }
            case Instr_Pass: {
                c->stack[c->sp++] = 1;
                c->steps = 0;
                break;
            }
            case Instr_Random: instr_random_int(c); break; 
            case Instr_Place: instr_place(c); break;
            case Instr_GoTo: change = instr_goto(c); break;
            case Instr_GoToIf: change = instr_goto_if(c); break;
            case Instr_Eq: change = instr_eq(c); break;
            case Instr_Lt: change = instr_lt(c); break;
            case Instr_Sub: change = instr_sub(c); break;
            case Instr_Add: change = instr_add(c); break;
            case Instr_Mul: change = instr_mul(c); break;
            case Instr_Div: change = instr_div(c); break;
            case Instr_Mod: change = instr_mod(c); break;
            case Instr_Not: change = instr_not(c); break;
            case Instr_Or: change = instr_or(c); break;
            case Instr_And: change = instr_and(c); break;
            case Instr_MoveSP: change = instr_move_sp(c); break;
            case Instr_Copy: change = instr_copy(c); break;
            case Instr_Swap: change = instr_swap(c); break;
            case Instr_Call: change = instr_call(c); break;
            case Instr_Return: change = instr_return(c); break;
            case Instr_Declare: change = instr_declare(c); break;
            case Instr_Index: change = instr_index(c); break;
            case Instr_BinOr: change = instr_binor(c); break;
            case Instr_BinNot: change = instr_binnot(c); break;
            case Instr_BinAnd: change = instr_binand(c); break;
            case Instr_LoadLocal: change = instr_load(c, c->bp); break;
            case Instr_StoreLocal: change = instr_store(c, c->bp); break;
            case Instr_BP: change = instr_bp(c); break;
            case Instr_Extract: change = instr_extract(c); break;
            case Instr_LoadGlobal: change = instr_load(c, 0); break;
            case Instr_StoreGlobal: change = instr_store(c, 0); break;
            case Instr_Meta: change = instr_meta(c); break;
            case Instr_Bits: change = instr_bits(c); break;
            case Instr_TCall: change = instr_tcall(c); break;
            default: break;
        }

        if (change) { 
            if (c->type == ENTITY_CONTEXT)
                // auto_viewport_player(c->entity); TODO: visual.h?
            print_board(); 
            wait(1); 
        }
        else if (_gs->feed_change) {
            print_board();
            wait(0.25);
        }


        if (
            (c->type == ENTITY_CONTEXT && !c->entity->active)
            || (c->type == EVENT_CONTEXT && c->event->cancelled)
        )
            break;
    }
    end:
    clean_up(c);
}