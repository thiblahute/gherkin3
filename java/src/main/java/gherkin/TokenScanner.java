package gherkin;

import gherkin.ast.Location;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;

public class TokenScanner implements Parser.ITokenScanner {

    private final BufferedReader reader;
    private int lineNumber;

    public TokenScanner(String source) {
        this(new StringReader(source));
    }

    public TokenScanner(Reader source) {
        this.reader = new BufferedReader(source);
    }

    @Override
    public Token read() throws IOException {
        String line = reader.readLine();
        Location location = new Location(++lineNumber, 0);
        if(line == null) {
            return new Token(null, location);
        }
        return new Token(new GherkinLine(line), location);
    }
}
