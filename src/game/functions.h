// functions.h - Guest functions the host calls or hooks.
//
// Names come from function_names.toml (generated), so they read
// pd_<module>_<function>. A hook defines pd_<name> with REX_HOOK_RAW and
// reaches the original through __imp__pd_<name>. Arguments are given as the game
// passes them (r3, r4, ...).
#pragma once

#include <rex/hook.h>

// player
REX_EXTERN(__imp__pd_player_remove_chr_body);   // ()
REX_EXTERN(__imp__pd_player_tick_third_person); // (prop)
REX_EXTERN(__imp__pd_player_move_camera_from_pos_rooms); // (pos, up, look, basepos, baserooms)
REX_EXTERN(__imp__pd_player_choose_third_person_animation); // (chr, crouchpos, ...)
REX_EXTERN(pd_player_tick_chr_body);            // ()

// playermgr: the body's held weapon
REX_EXTERN(pd_playermgr_delete_weapon); // (hand)
REX_EXTERN(pd_playermgr_create_weapon); // (hand)

// gun: the first-person weapon
REX_EXTERN(__imp__pd_gun_render);    // (&gdl)
REX_EXTERN(pd_gun_get_weapon_num);   // (hand) -> weapon
REX_EXTERN(pd_gun_free_gun_mem);     // () gunmemowner = FREE

// model
REX_EXTERN(pd_model_get_anim_num);  // (model) -> animnum
REX_EXTERN(pd_model_set_animation); // (model, animnum, flip; f1 startframe, f2 speed, f3 merge)

// portal / collision
REX_EXTERN(pd_portal_find_rooms); // (from, to, fromrooms, finalrooms, throughrooms, maxthrough)
REX_EXTERN(pd_collision_test_los_oobok_findclosest); // (from, fromrooms, to, types, geoflags)

// naudio: the N64 audio library
REX_EXTERN(__imp__pd_naudio_evtq_next_event); // (lock, evtq, evt) -> microseconds until evt

// platform (4J): profiles, sign-in, system message boxes
REX_EXTERN(__imp__pd_platform_show_message_box); // (box, title, text, button1, button2, user)

// xdk
REX_EXTERN(__imp__pd_xdk_ring_space_poll); // () -> nonzero while the ring has no space
