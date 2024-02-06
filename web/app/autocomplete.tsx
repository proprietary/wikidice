'use client';

import React from 'react';
import Downshift from 'downshift';
import { search } from '../api';

type AutocompleteProps = {
    query: string;
    placeholder: string;
    buttonText: string;
    onSearch?: (query: string) => Promise<Array<string>>;
    handleQueryChange: (query: string) => void;
    onSubmit: (query: string) => Promise<void>;
};

type AutocompleteState = {
    searchResults: Array<string>;
};

export class Autocomplete extends React.Component<AutocompleteProps> {
    public state: AutocompleteState;

    constructor(props: AutocompleteProps) {
        super(props);
        this.state = {
            searchResults: [],
        };
    }

    componentDidMount() {
        this.state = {
            searchResults: [],
        };
    }

    handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const query = e.target.value;
        this.props.handleQueryChange(query);
    }

    render() {
        return (
            <div>
                <Downshift
                    onStateChange={({ inputValue }) => {
                        if (inputValue != null) {
                            (this.props.onSearch || search)(inputValue).then((searchResults) => {
                                this.setState({ searchResults });
                            });
                        }
                    }}
                    inputValue={this.props.query}
                    onChange={selection => { this.props.handleQueryChange(selection); }}
                    itemToString={(item) => (item == null ? '' : item)}
                >
                    {({
                        getInputProps,
                        getItemProps,
                        getLabelProps,
                        getMenuProps,
                        isOpen,
                        highlightedIndex,
                        selectedItem,
                    }) => (
                        <div>
                            <label {...getLabelProps()}>Search</label>
                            <input
                                {...getInputProps({
                                    placeholder: "Physics",
                                    onChange: this.handleChange,
                                    value: this.props.query,
                                })}
                            />
                            <button onClick={() => this.props.onSubmit(this.props.query)}>{this.props.buttonText}</button>
                            <ul {...getMenuProps()}>
                                {isOpen
                                    ? this.state.searchResults
                                        .map((item, index) => (
                                            <li
                                                {...getItemProps({
                                                    key: index,
                                                    index,
                                                    item,
                                                    style: {
                                                        backgroundColor:
                                                            highlightedIndex === index ? 'lightgray' : 'white',
                                                        fontWeight: selectedItem === item ? 'bold' : 'normal',
                                                    },
                                                })}
                                            >
                                                {item}
                                            </li>
                                        ))
                                    : null}
                            </ul>
                        </div>
                    )}
                </Downshift>
            </div>
        );
    }
}