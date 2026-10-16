#ifndef M360_CHARACTER_SELECT_XDK_H
#define M360_CHARACTER_SELECT_XDK_H
#ifdef __cplusplus
extern "C" {
#endif
int M360_CharacterSelectEnter(unsigned mode);
void M360_CharacterSelectResume(void);
void M360_CharacterSelectAddKOs(const unsigned* counts);
int M360_CharacterSelectFrame(void); /* -1 pending, 0 cancel, 1 start */
int M360_CharacterSelectActive(void);
const char* M360_CharacterSelectNotice(void);
void* M360_CharacterSelectData(void);
void M360_CharacterSelectLeave(void);
#ifdef __cplusplus
}
#endif
#endif
