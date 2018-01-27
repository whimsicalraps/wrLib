#include "wrEvent.h"

#include "../stm32f7/1track/lib/debug_usart.h"

event_extract_t* Extract_init( void )
{
   event_extract_t* e = NULL;
   e = malloc(sizeof(event_extract_t));
   if( e == NULL ){ return NULL; } // failure

   // parameter defaults
   e->hyst_time      = 10;   // >= EXTRACT_HIST_LENGTH for debouncing

   // these constants are divided by EXTRACT_HIST_LENGTH
   // trigger detection
   e->tr_abs_level   = 0.25; // trigger level re:0v
   e->tr_rel_level   = 0.2;  // trigger level re:cv-level
   // trigger||cv thresholds
   e->cv_thresh_same = 0.2;  // cv-level re:0v (same dir as trigger)
   e->cv_thresh_opp  = 0.15; // cv-level re:0v (opposite dir)

   // state
   e->hyst = 0;
   for( uint8_t i=0; i<EXTRACT_HIST_LENGTH; i++ ){
      e->in_history[i] = 0.0;
   }
   e->tr_state = tr_none;

   return e;
}
void Extract_deinit( event_extract_t* e )
{
   free(e);
   e = NULL;
}
etrig_t Extract_cv_trigger( event_extract_t* e, float in )
{
   if( e->hyst == 0 ){
      if( in > (e->in_history[0] + e->tr_rel_level)
       && in > e->tr_abs_level ){
         // trigger high
         // replace these conditionals with some integer math
         // shift the cv, scale, then cast, then lookup in tr_t
         if( e->in_history[0] > e->cv_thresh_same ){
            e->tr_state = tr_p_positive;
         } else if( e->in_history[0] < -(e->cv_thresh_opp) ){
            e->tr_state = tr_p_negative;
         } else {
            e->tr_state = tr_p_same;
         }
         e->hyst = e->hyst_time;
      } else if( in < (e->in_history[0] - e->tr_rel_level)
              && in < -(e->tr_abs_level) ){
         // trigger low
         if( e->in_history[0] > e->cv_thresh_opp ){
            e->tr_state = tr_n_positive;
         } else if( e->in_history[0] < -(e->cv_thresh_same) ){
            e->tr_state = tr_n_negative;
         } else {
            e->tr_state = tr_n_same;
         }
         e->hyst = e->hyst_time;
      } else {
         // no trigger
         e->tr_state = tr_none;
      }
   } else {
      // Debug_USART_putn8(e->hyst);
      e->hyst--; if( e->hyst < 0 ){ e->hyst = 0; }
      e->tr_state = tr_none;
   }

   // circular buffer
   for( uint8_t i=0; i<EXTRACT_HIST_LENGTH - 1; i++ ){
      e->in_history[i] = e->in_history[i+1];
   }  e->in_history[EXTRACT_HIST_LENGTH - 1] = in;

   return e->tr_state;
}