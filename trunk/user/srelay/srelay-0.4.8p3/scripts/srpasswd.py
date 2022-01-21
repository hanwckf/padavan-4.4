#!/usr/bin/env python
"""Replacement for htpasswd -- apply to srelay """
# Original author: Eli Carter
#  md5, sha256 sha512 hashed pass can be generated
#                 if your python env is supported.
#   $Id: srpasswd.py,v 1.1 2017/08/25 05:53:27 bulkstream Exp $
#                                      Tomo.M
#
import os
import sys
import random
import hashlib, base64
from optparse import OptionParser

# We need a crypt module, but Windows doesn't have one by default.  Try to find
# one, and tell the user if we can't.
try:
    import crypt
except ImportError:
    try:
        import fcrypt as crypt
    except ImportError:
        sys.stderr.write("Cannot find a crypt module.  "
                         "Possibly http://carey.geek.nz/code/python-fcrypt/\n")
        sys.exit(1)

random.seed()


def salt(htype):
    """Returns a string of crypt salt according to hash type"""
    if htype == 'des':
        letters = 'abcdefghijklmnopqrstuvwxyz' \
            'ABCDEFGHIJKLMNOPQRSTUVWXYZ' \
            '0123456789/.'
        return random.choice(letters) + random.choice(letters)
    elif htype == 'md5':
            h = '$1$'
    elif htype == 'sha512':
            h = '$6$'
    else:
            h = '$5$'

    return h + base64.b64encode(hashlib.sha1(str(random.random())).digest(), "./")


def yes_no_input(msg="Please respond with 'yes' or 'no' [y/N]: "):
    while True:
        choice = raw_input(msg).lower()
        if choice in ['y', 'ye', 'yes']:
            return True
        elif choice in ['n', 'no']:
            return False

class HtpasswdFile:
    """A class for manipulating htpasswd files."""

    def __init__(self, filename, create=False):
        self.entries = []
        self.filename = filename
        if not create:
            if os.path.exists(self.filename):
                self.load()
            else:
                raise Exception("%s does not exist" % self.filename)
        else:
            if os.path.isfile(self.filename):
                yes = yes_no_input(msg="file: %s exists. overwrite? [y/N]: " % self.filename)
                if not yes:
                    print("Do nothing.")
                    exit(-1)

    def load(self):
        """Read the htpasswd file into memory."""
        lines = open(self.filename, 'r').readlines()
        self.entries = []
        for line in lines:
            username, pwhash = line.split(':')
            entry = [username, pwhash.rstrip()]
            self.entries.append(entry)

    def save(self):
        """Write the htpasswd file to disk"""
        open(self.filename, 'w').writelines(["%s:%s\n" % (entry[0], entry[1])
                                             for entry in self.entries])

    def update(self, username, password):
        """Replace the entry for the given user, or add it if new."""
        pwhash = crypt.crypt(password, salt(self.hash_type))
        matching_entries = [entry for entry in self.entries
                            if entry[0] == username]
        if matching_entries:
            matching_entries[0][1] = pwhash
        else:
            self.entries.append([username, pwhash])

    def delete(self, username):
        """Remove the entry for the given user."""
        self.entries = [entry for entry in self.entries
                        if entry[0] != username]


def main():
    """%prog [-c] -b filename username password
    Create or update an htpasswd file"""
    # For now, we only care about the use cases that affect tests/functional.py
    parser = OptionParser(usage=main.__doc__)
    parser.add_option('-b', action='store_true', dest='batch', default=False,
        help='Batch mode; password is passed on the command line IN THE CLEAR.'
        )
    parser.add_option('-c', action='store_true', dest='create', default=False,
        help='Create a new htpasswd file, overwriting any existing file.')
    parser.add_option('-D', action='store_true', dest='delete_user',
        default=False, help='Remove the given user from the password file.')
    parser.add_option('-t', '--type', action='store', dest='hash_type',
        default='sha256', help='Password Hash Type(md5, sha256, sha512, des: default=sha256).')

    options, args = parser.parse_args()

    def syntax_error(msg):
        """Utility function for displaying fatal error messages with usage
        help.
        """
        sys.stderr.write("Syntax error: " + msg)
        sys.stderr.write(parser.get_usage())
        sys.exit(1)

    if not options.batch:
        syntax_error("Only batch mode is supported\n")

    # Non-option arguments
    if len(args) < 2:
        syntax_error("Insufficient number of arguments.\n")
    filename, username = args[:2]
    if options.delete_user:
        if len(args) != 2:
            syntax_error("Incorrect number of arguments.\n")
        password = None
    else:
        if len(args) != 3:
            syntax_error("Incorrect number of arguments.\n")
        password = args[2]

    passwdfile = HtpasswdFile(filename, create=options.create)

    passwdfile.hash_type = options.hash_type

    if options.delete_user:
        passwdfile.delete(username)
    else:
        passwdfile.update(username, password)

    passwdfile.save()


if __name__ == '__main__':
    main()
