#-License----------------------------------------------------------------------
#Copyright (c) 2012, developer@teamboyce.com
#All rights reserved.
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice, 
#   this list of conditions and the following disclaimer in the documentation 
#   and/or other materials provided with the distribution.
# * Neither the name of the Team Boyce Limited nor the names of its contributors 
#   may be used to endorse or promote products derived from this software 
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

ifeq ($(TOPDIR),)
ifneq ($(wildcard ../Makefile),)
# If main system
$(if $(MAKECMDGOALS),$(MAKECMDGOALS),all):
	@make -C .. $@
else
GENDOSFS_STANDALONE := Y
endif
endif



TOOLS += gendosfs

gendosfs_SRC :=
gendosfs_SRC += gendosfs.c
gendosfs_SRC += src/ff.c
#gendosfs_SRC += src/option/unicode.c
gendosfs_SRC += src/option/syscall.c
gendosfs_SRC += src/option/ccsbcs.c
gendosfs_SRC += imageio.c

ifeq ($(GENDOSFS_STANDALONE),Y)

O := $(if $(O),$(O),build)


INC := $(addprefix -I,$(sort $(dir $(gendosfs_SRC))))

all: gendosfs

clean:
	rm -rf $(O) gendosfs

gendosfs: $(addprefix $(O)/,$(patsubst %.c,%.o,$(gendosfs_SRC)))
	 gcc -g -Wl,--start-group $^ -lpopt -Wl,--end-group -o $@


$(O)/%.o: %.c $(MAKEFILE_LIST)
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))
	gcc -g -std=gnu99 -pedantic -Wall $(INC) $<  -Wa,-adhlns="$(@:%.o=%.lst)" -MMD -MP -MF $(@:%.o=%.d) -MT $(@) -c -o $@


-include $(addprefix $(O)/,$(patsubst %.c,%.d,$(gendosfs_SRC)))
endif

