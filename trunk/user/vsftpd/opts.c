/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * opts.c
 *
 * Routines to handle OPTS.
 */

#include "ftpcodes.h"
#include "ftpcmdio.h"
#include "session.h"
#include "tunables.h"

void
handle_opts(struct vsf_session* p_sess)
{
  str_upper(&p_sess->ftp_arg_str);
   if (tunable_utf8 && str_equal_text(&p_sess->ftp_arg_str, "UTF8 ON"))
  {
    vsf_cmdio_write(p_sess, FTP_OPTSOK, "Always in UTF8 mode.");
  }
  else
  {
    vsf_cmdio_write(p_sess, FTP_BADOPTS, "Option not understood.");
  }
}

