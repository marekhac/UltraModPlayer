/* config.h for uade */

/* KDE2 Mimetype generation
 * To enable the generation of a KDE2 x-uade Filetype file
 * uncomment (and maybe edit) the following defines.
 * (It would be a good idea to have KDE2 really installed :)
 */

/*
#define KDE2_MIMETYPE "audio/x-amiga"
#define KDE2_LOCALDIR "~/.kde2/"
#define KDE2_UPDATE "kded --check"
*/


/* uncomment if you want to generate MIME-info entries for the 
 * ROX 1.2x filer [see http://rox.sf.net/] from the configuration 
 * of the xmms input plugin.
 */

/*
#define ROX_MIMETYPE "audio/x-amiga"
#define ROX_MIMEINFO "~/Choices/MIME-info/uade.rules"
*/


/* uncomment if you want to generate MIME  entries for the 
 * shared mime info from freedesktop.org (for e.g ROX 1.3.x)
 * from the configuration of the xmms input plugin.
 *
 * You need to have the shared-mime-info package from
 * http://www.freedesktop.org/standards/shared-mime-info
 * installed.
 */

/*
#define SMI_MIMETYPE "audio/x-amiga"
#define SMI_MIMEINFO "~/.mime/packages/UADE.xml"
#define SMI_UPDATE "update-mime-database ~/.mime/"
*/


/* XPK Lib for GNU/Linux support
 * If you have the xpk library for gnu/linux installed and
 * want to use this instead of the native xpk-sqsh support
 * uncomment the following line
 */

/*
#define HAVE_XPKLIB 1
*/


/* Powerpacker data file decrypting */
/*
#define WANT_PP2X_DECRYPTING 1
*/


/* increase buffer if you use SDL audio output (e.g. Mac OS X) and
   audio stutters while moving windows around and so on
    8192 < n < 16384 (must be a power of 2)
*/

#define SDL_MAXBSIZE 16384

#define HAVE_AMIGA_SHELL
