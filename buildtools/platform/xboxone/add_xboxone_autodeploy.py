#!/usr/bin/pyton
import os
import sys
import argparse


def updateSln(slnfilepath, project):
    """ Injects auto-deployment entires for XboxOne into the solution file
        for the specified projects.

        * Expects all Project entries in the solution do be listed before the Global section.
        * Does not check if the projects have auto-deployment set already,
          so should be used only once after the solution was generated.

        Tested with Microsoft Visual Studio Solution File, Format Version 12.00, Visual Studio 14
    """
    if not os.path.exists(slnfilepath):
        sys.exit(slnfilepath + " not found")
        
    guid = {}
    entries = []
    found = 0
    configs = ["debug","checked","profile","release"]

    tmpfilepath = slnfilepath + ".tmp"

    slnfile = open(slnfilepath, "r")
    tmpfile = open(tmpfilepath, "w")
    
    print "updating " + slnfilepath

    # scan for the projects while copying to tmp file
    for line in slnfile:
        if(line.startswith("Project")):
            projname = line.split()[2].strip('",')
            for p in project:
                if(projname == p):
                    guid[projname] = line.split(",")[2].strip().strip('"')
                    project.remove(projname)
                    print "found project " + projname + " " + guid[projname]

        tmpfile.write(line)

        # all projects should have been found by now,
        # this is the section where the new entries are injected
        if(line.find("GlobalSection(ProjectConfigurationPlatforms) = postSolution") > -1):
            break;
        
    # build the new entries
    for g in guid.values():
        for c in configs:
            entries.append(g + "." + c + "|Durango.Deploy.0 = " + c + "|Durango")

    # write them to the tmp file
    for e in entries:
        tmpfile.write('\t\t' + e + '\n')

    # dump the rest of the original file
    for line in slnfile:
        tmpfile.write(line)
        
    # replace the original file
    slnfile.close()
    tmpfile.close()
    os.remove(slnfilepath)
    os.rename(tmpfilepath, slnfilepath)

    if (len(project) > 0):
        sys.exit("expected project not found: " + str(project))
        

def main(argv=None):
    parser = argparse.ArgumentParser(description="Adds XboxOne deployment to a solution's projects")
    parser.add_argument("solution", help="path to solution file")
    parser.add_argument("project", nargs='+', help="pattern name of the project")
    args = parser.parse_args(argv)
    updateSln(args.solution, args.project)


if __name__ == "__main__":
    main()
